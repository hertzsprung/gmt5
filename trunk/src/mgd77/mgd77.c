/*---------------------------------------------------------------------------
 *	$Id: mgd77.c,v 1.58 2005-10-16 09:17:53 pwessel Exp $
 *
 *    Copyright (c) 2005 by P. Wessel
 *    See README file for copying and redistribution conditions.
 *
 *  File:	mgd77.c
 *
 *  Functino library for programs that plan to read/write MGD77[+] files
 *
 *  Authors:    Paul Wessel, Primary Investigator, SOEST, U. of Hawaii
 *		Michael Chandler, Master's Candidate, SOEST, U. of Hawaii
 *		
 *  Version:	1.1
 *  Revised:	10-OCT-2005
 * 
 *-------------------------------------------------------------------------*/

#include "mgd77.h"
#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN32) || defined(__EMX__)  /* Some definitions and includes are different under Windows or OS/2 */
#define STAT _stat
#else                                   /* Here for Unix, Linux, Cygwin, Interix, etc */
#define STAT stat
#endif

#define MGD77_CDF_CONVENTION	"COARDS"	/* MGD77+ files are COARDS-compliant */

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
int MGD77_Read_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);	  /* Will read a single tabular MGD77 record */
int MGD77_Write_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);	  /* Will read a single tabular MGD77 record */
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
int MGD77_Verify_Header (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P);

struct MGD77_DATA_RECORD *MGD77Record;
 
struct MGD77_RECORD_DEFAULTS mgd77defs[MGD77_N_DATA_FIELDS] = {
#include "mgd77defaults.h"
};

BOOLEAN MGD77_format_allowed[MGD77_N_FORMATS] = {TRUE, TRUE, TRUE};	/* By default we allow opening of files in any format.  See MGD77_Ignore_Format() */

double MGD77_NaN_val[7], MGD77_Low_val[7], MGD77_High_val[7];
double MGD77_Epoch_zero;
char *MGD77_suffix[MGD77_N_FORMATS] = {"mgd77", "nc", "dat"};

struct MGD77_LIMITS {
	double limit[2];	/* Upper and lower range */
} mgd77_range[MGD77_N_DATA_EXTENDED];

struct MGD77_cdf {
	int type;		/* netCDF variable type */
	int len;		/* # of characters (if text), 1 otherwise */
	double scale;		/* scale to multiply stored data to get correct magnitude */
	double offset;		/* offset to add after multiplication */
	char *units;		/* Units of this data */
	char *comment;		/* Comments regarding this data */
};

struct MGD77_cdf mgd77cdf[MGD77_N_DATA_EXTENDED] = {
	{ NC_BYTE,	1,	1.0,	0.0, "", "Normally 5" },
	{ NC_BYTE,	1,	1.0,	0.0, "hours", "-13 to +12 inclusive" },
	{ NC_BYTE,	1,	1.0,	0.0, "year", "Year of the survey" },
	{ NC_BYTE,	1,	1.0,	0.0, "month", "1 to 12 inclusive" },
	{ NC_BYTE,	1,	1.0,	0.0, "day", "1 to 31 inclusive" },
	{ NC_BYTE,	1,	1.0,	0.0, "hour", "0 to 23 inclusive" },
	{ NC_BYTE,	1,	1.0,	0.0, "min", "Decimal minutes with 0.001 precision, 0 to 59.999" },
	{ NC_INT,	1,	1.0e-6,	0.0, "degrees_north", "Negative south of Equator" },
	{ NC_INT,	1,	1.0e-6,	0.0, "degrees_east", "Negative west of Greenwich" },
	{ NC_BYTE,	1,	1.0,	0.0, "", "Observed (1), Interpolated (3), or Unspecified (9)" },
	{ NC_INT,	1,	1.0e-4,	0.0, "second", "Corrected for transducer depth, etc." },
	{ NC_INT,	1,	1.0e-1,	0.0, "meter", "Corrected for sound velocity variations (if known)" },
	{ NC_BYTE,	1,	1.0,	0.0, "", "01-55 (= Matthew's zone), 59 (Matthew's zone unknown), 60 (Kuwahara), 61 (Wilson), 62 (Del Grosso) 63 (Carter), 88 (Other; see header), 98 (Unknown), or 99 (Unspecified)" },
	{ NC_BYTE,	1,	1.0,	0.0, "", "Observed (1), Interpolated (3), or Unspecified (9)" },
	{ NC_INT,	1,	1.0e-1,	0.0, "nTesla", "Leading sensor" },
	{ NC_INT,	1,	1.0e-1,	0.0, "nTesla", "Trailing sensor" },
	{ NC_SHORT,	1,	1.0e-1,	0.0, "nTesla", "Corrected for reference field (see header)" },
	{ NC_BYTE,	1,	1.0,	0.0, "", "Magnetic sensor used: 1, 2, or Unspecified (9)" },
	{ NC_SHORT,	1,	1.0e-1,	0.0, "nTesla", "Already applied to data" },
	{ NC_SHORT,	1,	1.0,	0.0, "meter", "Positive below sealevel" },
	{ NC_INT,	1,	1.0e-1,	0.0, "mGal", "Corrected for Eotvos, drift, and tares" },
	{ NC_SHORT,	1,	1.0e-1,	0.0, "mGal", "7.5 V cos (lat) sin (azim) + 0.0042 V*V" },
	{ NC_SHORT,	1,	1.0e-1,	0.0, "mGal", "Observed - theoretical" },
	{ NC_BYTE,	1,	1.0,	0.0, "", "Suspected by (5) source agency, (6) NGDC, or no problems found (9)" },
	{ NC_BYTE,	8,	1.0,	0.0, "", "Identical to ID in header" },
	{ NC_BYTE,	5,	1.0,	0.0, "", "For cross-referencing with seismic data" },
	{ NC_BYTE,	6,	1.0,	0.0, "", "For cross-referencing with seismic data" },
	{ NC_DOUBLE,	1,	1.0,	0.0, "seconds since 1970-01-01 00:00:00 0", "GMT Unix time, subtract TZ to get ship local time" }
};	

BOOLEAN MGD77_Strip_Blanks = FALSE;
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
int MGD77_Info_from_Abbrev (char *name, struct MGD77_HEADER *H, int *key);

#include "mgd77_functions.h"	/* Get netCDF MGD77 header attribute i/o functions */

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
			exit (EXIT_FAILURE);
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
	int error, k;
	
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
	
	k = MGD77_Verify_Header (F, H->mgd77);
	if (k) fprintf (stderr, "%s: %d header errors found\n", GMT_program, k);
	
	return (error);
}

int MGD77_Write_Header_Record (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* Writes the header structgure to a MGD77[+] file */
	int error;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Will read MGD77 headers from MGD77 files or ascii tables */
		case MGD77_FORMAT_TBL:
			error = MGD77_Write_Header_Record_m77 (file, F, H);
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
			for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) dvals[i] = MGD77Record.number[i];
			dvals[MGD77_TIME] = MGD77Record.time;
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (tvals[i], MGD77Record.word[k]);
			break;
		case MGD77_FORMAT_CDF:		/* Will read a single MGD77+ netCDF record */
			error = MGD77_Read_Data_Record_cdf (F, H, dvals, tvals);
			break;
		case MGD77_FORMAT_TBL:		/* Will read a single ascii table record */
			error = MGD77_Read_Data_Record_tbl (F, &MGD77Record);
			for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) dvals[i] = MGD77Record.number[i];
			dvals[MGD77_TIME] = MGD77Record.time;
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (tvals[i], MGD77Record.word[k]);
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
			for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) MGD77Record.number[i] = dvals[i];
			MGD77Record.time = dvals[MGD77_TIME];
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (MGD77Record.word[k], tvals[i]);
			error = MGD77_Write_Data_Record_m77 (F, &MGD77Record);
			break;
		case MGD77_FORMAT_CDF:		/* Will write a single MGD77+ netCDF record */
			error = MGD77_Write_Data_Record_cdf (F, H, dvals, tvals);
			break;
		case MGD77_FORMAT_TBL:		/* Will write a single ascii table record; first fill out MGD77_RECORD structure */
			for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) MGD77Record.number[i] = dvals[i];
			MGD77Record.time = dvals[MGD77_TIME];
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (MGD77Record.word[k], tvals[i]);
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
	char *MGD77_header[MGD77_N_HEADER_RECORDS];
	int sequence, err;
	struct STAT buf;
	
	/* argument file is generally ignored since file is already open */
	
	memset ((void *)H, '\0', sizeof (struct MGD77_HEADER));	/* Completely wipe existing header */
	if (F->format == MGD77_FORMAT_M77) {			/* Can compute # records from file size because format is fixed */
		if (stat (F->path, &buf)) {	/* Inquiry about file failed somehow */
			fprintf (stderr, "%s: Unable to stat file %s\n", GMT_program, F->path);
			exit (EXIT_FAILURE);
		}
		/* Not tested under Windoze: Do we use +2 because of \r\n ? */
		H->n_records = irint ((double)(buf.st_size - (MGD77_N_HEADER_RECORDS * (MGD77_HEADER_LENGTH + 1))) / (double)(MGD77_RECORD_LENGTH + 1));
	}
	else {
		char line[BUFSIZ];
		/* Since we do not know the number of records, we must quickly count lines */
		while (fgets (line, BUFSIZ, F->fp)) H->n_records++;	/* Count every line */
		rewind (F->fp);						/* Go back to beginning of file */
		H->n_records -= MGD77_N_HEADER_RECORDS;			/* Adjust for the 24 records in the header block */
	}
	
	/* Read Sequences No 01-24: */

	for (sequence = 0; sequence < MGD77_N_HEADER_RECORDS; sequence++) {
		MGD77_header[sequence] = (char *)GMT_memory (VNULL, MGD77_HEADER_LENGTH + 1, sizeof (char), GMT_program);
		if ((err = MGD77_Read_Header_Sequence (F->fp, MGD77_header[sequence], sequence+1))) return (err);
	}
	
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
	MGD77_Place_Text (dir, P->Paramaters_Surveyed_Code, record[k], 27, 5);
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

int MGD77_Verify_Header (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P)
{
	int i, k, pos, w, e, s, n, err = 0;
	char copy[151], p[GMT_TEXT_LEN], *pscode[5] = {"Bathy", "Magnetics", "Gravity", "3.5 kHz", "Seismics"};
	time_t now;
	FILE *fp;
	struct tm *T;
	
	if (!F->header_verify_level) return (0);	/* Header verification not desired */
	
	fp = (F->header_verify_level == 1) ? GMT_stdout : stderr;
	
	(void) time (&now);
	
	T = gmtime (&now);
	
	/* Verify Sequence No 01: */
	
	if (!(P->Record_Type == '1' || P->Record_Type == '4')) {
		fprintf (fp, "#H-%s-01-01: Invalid Record Type: Found (%c) : Expected :(4)\n", F->NGDC_id, P->Record_Type);
		err++;
	}
	if (!P->Survey_Identifier[0]) {
		fprintf (fp, "#H-%s-01-02: Survey Identifier: Found (%s) : Expected :(XXXXXXXX)\n", F->NGDC_id, P->Survey_Identifier);
		err++;
	}
	if (strcmp (P->Format_Acronym, "MGD77")) {
		fprintf (fp, "#H-%s-01-03: Format Acronym: Found (%s) : Expected :(MGD77)\n", F->NGDC_id, P->Format_Acronym);
		err++;
	}
	if (strcmp (P->Data_Center_File_Number,F->NGDC_id)) {
		fprintf (fp, "#H-%s-01-04: Data Center File Number: Found (%s) : Expected :(%s)\n", F->NGDC_id, P->Data_Center_File_Number, F->NGDC_id);
		err++;
	}
	for (i = 0; i < 5; i++) {
		if (P->Paramaters_Surveyed_Code[i] == ' ') continue;	/* Skip the OK codes */
		if (P->Paramaters_Surveyed_Code[i] == '0') continue;
		if (P->Paramaters_Surveyed_Code[i] == '1') continue;
		if (P->Paramaters_Surveyed_Code[i] == '3') continue;
		if (P->Paramaters_Surveyed_Code[i] == '5') continue;
		fprintf (fp, "#H-%s-01-%2.2d: Parameter Survey Code [%s]: (Found (%c) : Expected :( )\n", F->NGDC_id, 5 + i, pscode[i], P->Paramaters_Surveyed_Code[i]);
		err++;
	}
	if (P->File_Creation_Year[0] && ((i = atoi (P->File_Creation_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year))) {
		fprintf (fp, "#H-%s-01-10: File Creation Year: Found (%s) : Expected :(    )\n", F->NGDC_id, P->File_Creation_Year);
		err++;
	}
	if (P->File_Creation_Month[0] && ((i = atoi (P->File_Creation_Month)) < 1 || i > 12)) {
		fprintf (fp, "#H-%s-01-10: File Creation Month: Found (%s) : Expected :(  )\n", F->NGDC_id, P->File_Creation_Month);
		err++;
	}
	if (P->File_Creation_Day[0] && ((i = atoi (P->File_Creation_Day)) < 1 || i > 31)) {
		fprintf (fp, "#H-%s-01-10: File Creation Day: Found (%s) : Expected :(  )\n", F->NGDC_id, P->File_Creation_Day);
		err++;
	}

	/* Verify Sequence No 02: */

	if (P->Platform_Type_Code < '0' || P->Platform_Type_Code > '9') {
		fprintf (fp, "#H-%s-02-03: Invalid Platform Type Code: Found (%c) : Expected :(0)\n", F->NGDC_id, P->Platform_Type_Code);
		err++;
	}

	/* Verify Sequence No 04: */

	if (P->Survey_Departure_Year[0] && ((i = atoi (P->Survey_Departure_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year))) {
		fprintf (fp, "#H-%s-04-01: Survey Departure Year: Found (%s) : Expected :(    )\n", F->NGDC_id, P->Survey_Departure_Year);
		err++;
	}
	if (P->Survey_Departure_Month[0] && ((i = atoi (P->Survey_Departure_Month)) < 1 || i > 12)) {
		fprintf (fp, "#H-%s-04-02: Survey Departure Month: Found (%s) : Expected :(  )\n", F->NGDC_id, P->Survey_Departure_Month);
		err++;
	}
	if (P->Survey_Departure_Day[0] && ((i = atoi (P->Survey_Departure_Day)) < 1 || i > 31)) {
		fprintf (fp, "#H-%s-04-03: Survey Departure Day: Found (%s) : Expected :(  )\n", F->NGDC_id, P->Survey_Departure_Day);
		err++;
	}
	if (P->Survey_Arrival_Year[0] && ((i = atoi (P->Survey_Arrival_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year))) {
		fprintf (fp, "#H-%s-04-04: Survey Arrival Year: Found (%s) : Expected :(    )\n", F->NGDC_id, P->Survey_Arrival_Year);
		err++;
	}
	if (P->Survey_Arrival_Month[0] && ((i = atoi (P->Survey_Arrival_Month)) < 1 || i > 12)) {
		fprintf (fp, "#H-%s-04-05: Survey Arrival Month: Found (%s) : Expected :(  )\n", F->NGDC_id, P->Survey_Arrival_Month);
		err++;
	}
	if (P->Survey_Arrival_Day[0] && ((i = atoi (P->Survey_Arrival_Day)) < 1 || i > 31)) {
		fprintf (fp, "#H-%s-04-06: Survey Arrival Day: Found (%s) : Expected :(  )\n", F->NGDC_id, P->Survey_Arrival_Day);
		err++;
	}

	/* Verify Sequence No 10: */

	if (P->Format_Type != 'A') {
		fprintf (fp, "#H-%s-10-01: Invalid Format Type: Found (%c) : Expected :(A)\n", F->NGDC_id, P->Format_Type);
		err++;
	}
	strcpy (copy, P->Format_Description);
	GMT_str_toupper (copy);
	if (strcmp (copy, "(I1,A8,I3,I4,3I2,F5.3,F8.5,F9.5,I1,F6.4,F6.1,I2,I1,3F6.1,I1,F5.1,F6.0,F7.1,F6.1,F5.1,A5,A6,I1)")) {
		fprintf (fp, "#H-%s-10-01: Invalid Format Description: Found (%s) : Expected :((I1,A8,I3,I4,3I2,F5.3,F8.5,F9.5,I1,F6.4,F6.1,I2,I1,3F6.1,I1,F5.1,F6.0,F7.1,F6.1,F5.1,A5,A6,I1))\n", F->NGDC_id, P->Format_Description);
		err++;
	}

	/* Process Sequence No 11: */

	w = e = s = n = 9999;
	if (P->Topmost_Latitude[0] && ((n = atoi (P->Topmost_Latitude)) < -90 || n > +90)) {
		fprintf (fp, "#H-%s-11-02: Topmost Latitude outside range: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Topmost_Latitude);
		err++;
	}
	if (P->Bottommost_Latitude[0] && ((s = atoi (P->Bottommost_Latitude)) < -90 || s > +90)) {
		fprintf (fp, "#H-%s-11-03: Bottommost Latitude: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Bottommost_Latitude);
		err++;
	}
	if (!(s == 9999 || n == 9999) && s > n) {
		fprintf (fp, "#H-%s-11: Bottommost Latitude %d exceeds Topmost Latitude %d\n", F->NGDC_id, s, n);
		err++;
	}
	if (P->Leftmost_Longitude[0] && ((w = atoi (P->Leftmost_Longitude)) < -180 || w > +180)) {
		fprintf (fp, "#H-%s-11-04: Leftmost Longitude: Found (%s) : Expected :(    )\n", F->NGDC_id, P->Leftmost_Longitude);
		err++;
	}
	if (P->Rightmost_Longitude[0] && ((e = atoi (P->Rightmost_Longitude)) < -180 || e > +180)) {
		fprintf (fp, "#H-%s-11-05: Rightmost Longitude: Found (%s) : Expected :(    )\n", F->NGDC_id, P->Rightmost_Longitude);
		err++;
	}
	if (!(w == 9999 || e == 9999) && w > e) {
		fprintf (fp, "#H-%s-11: Leftmost Longitude %d exceeds Rightmost Longitude %d\n", F->NGDC_id, w, e);
		err++;
	}

	/* Process Sequence No 12: */

	if (P->Bathymetry_Digitizing_Rate[0] && ((i = atoi (P->Bathymetry_Digitizing_Rate)) <= 0 || i >= 300)) {	/* 30 min */
		fprintf (fp, "#H-%s-12-01: Bathymetry Digitizing Rate: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Bathymetry_Digitizing_Rate);
		err++;
	}
	if (P->Bathymetry_Assumed_Sound_Velocity[0] && !((i = atoi (P->Bathymetry_Assumed_Sound_Velocity)) == 14630 || i == 15000)) {
		fprintf (fp, "#H-%s-12-03: Bathymetry Assumed Sound Velocity: Found (%s) : Expected :(15000)\n", F->NGDC_id, P->Bathymetry_Assumed_Sound_Velocity);
		err++;
	}
	if (P->Bathymetry_Datum_Code[0]) {
		i = atoi (P->Bathymetry_Datum_Code);
		if (!((i >= 0 && i <= 11) || i == 88)) {
			fprintf (fp, "#H-%s-12-04: Bathymetry Datum Code: Found (%s) : Expected :(00)\n", F->NGDC_id, P->Bathymetry_Datum_Code);
			err++;
		}
	}

	/* Process Sequence No 13: */

	if (P->Magnetics_Digitizing_Rate[0] && ((i = atoi (P->Magnetics_Digitizing_Rate)) < 0 || i >= 300)) {	/* 30 m */
		fprintf (fp, "#H-%s-13-01: Magnetics Digitizing Rate: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Magnetics_Digitizing_Rate);
		err++;
	}
	if (P->Magnetics_Sampling_Rate[0] && ((i = atoi (P->Magnetics_Sampling_Rate)) < 0 || i >= 30)) {
		fprintf (fp, "#H-%s-13-02: Magnetics Sampling Rate: Found (%s) : Expected :(  )\n", F->NGDC_id, P->Magnetics_Sampling_Rate);
		err++;
	}
	if (P->Magnetics_Sensor_Tow_Distance[0] && ((i = atoi (P->Magnetics_Sensor_Tow_Distance)) < 0)) {
		fprintf (fp, "#H-%s-13-03: Magnetics Sensor Tow Distance: Found (%s) : Expected :(    )\n", F->NGDC_id, P->Magnetics_Sensor_Tow_Distance);
		err++;
	}
	if (P->Magnetics_Sensor_Depth[0] && ((i = atoi (P->Magnetics_Sensor_Depth)) < 0)) {
		fprintf (fp, "#H-%s-13-04: Magnetics Sensor Depth: Found (%s) : Expected :(     )\n", F->NGDC_id, P->Magnetics_Sensor_Depth);
		err++;
	}
	if (P->Magnetics_Sensor_Separation[0] && ((i = atoi (P->Magnetics_Sensor_Separation)) < 0)) {
		printf ("%s: Magnetics Sensor Separation outside range: (%s)\n", F->NGDC_id, P->Magnetics_Sensor_Separation);
		fprintf (fp, "#H-%s-13-05: Magnetics Sensor Separation: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Magnetics_Sensor_Separation);
		err++;
	}
	i = 0;
	if (P->Magnetics_Ref_Field_Code[0]) {
		i = atoi (P->Magnetics_Ref_Field_Code);
		if (!((i >= 0 && i <= 13) || i == 88)) {
			fprintf (fp, "#H-%s-13-06: Magnetics Reference Field Code: Found (%s) : Expected :(00)\n", F->NGDC_id, P->Magnetics_Ref_Field_Code);
			err++;
		}
	}
	if (P->Magnetics_Ref_Field[0] && i == 88) {
		fprintf (fp, "#H-%s-13: Magnetics Ref Code == 88 but no Ref Field specified\n", F->NGDC_id);
		err++;
	}

	/* Process Sequence No 14: */

	if (P->Gravity_Digitizing_Rate[0] && ((i = atoi (P->Gravity_Digitizing_Rate)) < 0 || i > 300)) {	/* 30 m */
		fprintf (fp, "#H-%s-14-01: Gravity Digitizing Rate: Found (%s) : Expected :(   )\n", F->NGDC_id, P->Gravity_Digitizing_Rate);
		err++;
	}
	if (P->Gravity_Sampling_Rate[0] && ((i = atoi (P->Gravity_Sampling_Rate)) < 0 || i > 98)) {
		fprintf (fp, "#H-%s-14-02: Gravity Sampling Rate: Found (%s) : Expected :(00)\n", F->NGDC_id, P->Gravity_Digitizing_Rate);
		err++;
	}
	i = P->Gravity_Theoretical_Formula_Code - '0';
	if (P->Gravity_Theoretical_Formula_Code && !((i >= 1 && i <= 4) || i == 8)) {
		fprintf (fp, "#H-%s-14-03: Gravity Theoretical Formula Code: Found (%c) : Expected :( )\n", F->NGDC_id, P->Gravity_Theoretical_Formula_Code);
		err++;
	}
	i = P->Gravity_Reference_System_Code - '0';
	if (P->Gravity_Reference_System_Code && !((i >= 1 && i <= 3) || i == 9)) {
		fprintf (fp, "#H-%s-14-05: Gravity Reference System Code: Found (%c) : Expected :( )\n", F->NGDC_id, P->Gravity_Reference_System_Code);
		err++;
	}

	/* Process Sequence No 15: */

	if (P->Gravity_Departure_Base_Station[0] && ((i = atoi (P->Gravity_Departure_Base_Station)) < 9700000 || i > 9820000)) {
		fprintf (fp, "#H-%s-15-01: Gravity Departure Base Station Value: Found (%s) : Expected :(       )\n", F->NGDC_id, P->Gravity_Departure_Base_Station);
		err++;
	}
	if (P->Gravity_Arrival_Base_Station[0] && ((i = atoi (P->Gravity_Arrival_Base_Station)) < 9700000 || i > 9820000)) {
		fprintf (fp, "#H-%s-15-03: Gravity Arrival Base Station Value: Found (%s) : Expected :(       )\n", F->NGDC_id, P->Gravity_Arrival_Base_Station);
		err++;
	}

	/* Process Sequence No 16: */

	n = 0;
	if (P->Number_of_Ten_Degree_Identifiers[0] && ((n = atoi (P->Number_of_Ten_Degree_Identifiers)) < 1 || n > 30)) {
		fprintf (fp, "#H-%s-16-01: Number of Ten Degree Identifiers: Found (%s) : Expected :(       )\n", F->NGDC_id, P->Number_of_Ten_Degree_Identifiers);
		err++;
	}
	pos = 0;
	strcpy (copy, P->Ten_Degree_Identifier);
	while (GMT_strtok (copy,",", &pos, p)) {
		if (!strcmp (p, "9999")) {
			if (n && (pos-1) != n) {
				fprintf (fp, "#H-%s-16: Number of Ten Degree Identifiers: Found (%d) : Expected :(%d)\n", F->NGDC_id, pos - 1, n);
				n = 0;
			}
			continue;
		}
		if (!strcmp (p, "   0")) continue;
		k = 0;
		if (!(p[0] == '1' || p[0] == '3' || p[0] == '5' || p[0] == '7')) {
			printf ("#H-%s-16: Unknown Ten Degree Identifier quadrant: (%s)\n", F->NGDC_id, p);
			k++;
		}
		if (!(p[1] >= '0' && p[1] <= '9')) {
			printf ("#H-%s-16: Unknown Ten Degree Identifier latitude value: (%s)\n", F->NGDC_id, p);
			k++;
		}
		if ((i = atoi (&p[2])) < 0 || i > 18) {
			printf ("#H-%s-16: Unknown Ten Degree Identifier lonitude value: (%s)\n", F->NGDC_id, p);
			k++;
		}
		if (k) fprintf (fp, "#H-%s-%2.2d-02: Ten Degree Identifier # %d: Found (%s) : Expected :(9999)\n", F->NGDC_id, 16 + pos/15, pos, p);
		err += k;
	}

	return (err);
}

void MGD77_Place_Text (int dir, char *struct_member, char *header_record, int start_pos, int n_char)
{	/* Pos refers to position in the Fortran punch card, ranging from 1-80.
	 * We either copy from header to structure member or the other way. */
	int i;
	BOOLEAN strip_trailing_spaces;
	
	strip_trailing_spaces = !(dir & 32);
	dir &= 31;	/* Knowck of 32 flag if present */
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
	int i;
	
	/* When reading a plain ASCII MGD77 file we must set the information structure manually here */
	
	for (i = 0; i < MGD77_SET_COLS; i++) H->info[MGD77_M77_SET].col[i].present = H->info[MGD77_CDF_SET].col[i].present = FALSE;

	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {
		H->info[MGD77_M77_SET].col[i].abbrev = MGD77_cp_txt (mgd77defs[i].abbrev);
		H->info[MGD77_M77_SET].col[i].name = MGD77_cp_txt (mgd77defs[i].fieldID);
		H->info[MGD77_M77_SET].col[i].units = MGD77_cp_txt (mgd77cdf[i].units);
		H->info[MGD77_M77_SET].col[i].comment = MGD77_cp_txt (mgd77cdf[i].comment);
		H->info[MGD77_M77_SET].col[i].scale = mgd77cdf[i].scale;
		H->info[MGD77_M77_SET].col[i].offset = mgd77cdf[i].offset;
		H->info[MGD77_M77_SET].col[i].type = mgd77cdf[i].type;
		H->info[MGD77_M77_SET].col[i].text = 0;
		H->info[MGD77_M77_SET].col[i].pos = i;
		H->info[MGD77_M77_SET].col[i].present = TRUE;
	}
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {
		H->info[MGD77_M77_SET].col[i].abbrev = MGD77_cp_txt (mgd77defs[i].abbrev);
		H->info[MGD77_M77_SET].col[i].name = MGD77_cp_txt (mgd77defs[i].fieldID);
		H->info[MGD77_M77_SET].col[i].units = MGD77_cp_txt (mgd77cdf[i].units);
		H->info[MGD77_M77_SET].col[i].comment = MGD77_cp_txt (mgd77cdf[i].comment);
		H->info[MGD77_M77_SET].col[i].scale = 1.0;
		H->info[MGD77_M77_SET].col[i].offset = 0.0;
		H->info[MGD77_M77_SET].col[i].type = mgd77cdf[i].type;
		H->info[MGD77_M77_SET].col[i].text = mgd77cdf[i].len;
		H->info[MGD77_M77_SET].col[i].pos = i;
		H->info[MGD77_M77_SET].col[i].present = TRUE;
	}
	/* Finally, do the time field */
	H->info[MGD77_M77_SET].col[MGD77_TIME].abbrev = MGD77_cp_txt ("time");
	H->info[MGD77_M77_SET].col[MGD77_TIME].name = MGD77_cp_txt ("GMT J2000 Time");
	H->info[MGD77_M77_SET].col[MGD77_TIME].units = MGD77_cp_txt (mgd77cdf[MGD77_TIME].units);
	H->info[MGD77_M77_SET].col[MGD77_TIME].comment = MGD77_cp_txt (mgd77cdf[MGD77_TIME].comment);
	H->info[MGD77_M77_SET].col[MGD77_TIME].scale = mgd77cdf[MGD77_TIME].scale;
	H->info[MGD77_M77_SET].col[MGD77_TIME].offset = mgd77cdf[MGD77_TIME].offset;
	H->info[MGD77_M77_SET].col[MGD77_TIME].type = mgd77cdf[MGD77_TIME].type;
	H->info[MGD77_M77_SET].col[MGD77_TIME].text = 0;
	H->info[MGD77_M77_SET].col[MGD77_TIME].pos = MGD77_TIME;
	H->info[MGD77_M77_SET].col[MGD77_TIME].present = TRUE;
	H->n_fields = H->info[MGD77_M77_SET].n_col = MGD77_N_DATA_FIELDS + 1;
}

int MGD77_Read_Header_Record_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)  /* Will read the entire 24-section header structure */
{
	int id, c, c_id[2], n_dims, err, n_vars, dims[2];
	size_t count[2] = {0, 0}, length;
	char name[32];
	double corr_factor, corr_offset;
	
	if (MGD77_Open_File (file, F, MGD77_READ_MODE)) return (-1);			/* Basically sets the path */
	
	MGD77_nc_status (nc_open (F->path, NC_NOWRITE, &F->nc_id));	/* Open the file */
	
	/* Get the basic MGD77 header records */
	
	memset ((void *)H, 0, sizeof (struct MGD77_HEADER));
	
	MGD77_nc_status (nc_inq_attlen (F->nc_id, NC_GLOBAL, "Author", count));					/* Get length of author */
	H->author = (char *) GMT_memory (VNULL, count[0] + 1, sizeof (char), "MGD77_Read_Header_Record_cdf");	/* Get memory for author */
	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "Author",  H->author));
	MGD77_nc_status (nc_inq_attlen (F->nc_id, NC_GLOBAL, "history", count));				/* Get length of history */
	H->history = (char *) GMT_memory (VNULL, count[0] + 1, sizeof (char), "MGD77_Read_Header_Record_cdf");	/* Get memory for history */
	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "history", H->history));
	H->history[count[0]] = '\0';
	H->mgd77 = (struct MGD77_HEADER_PARAMS *) GMT_memory (VNULL, 1, sizeof (struct MGD77_HEADER_PARAMS), GMT_program);	/* Allocate parameter header */
	MGD77_Read_Header_Params (F, H->mgd77);	/* Get all the MGD77 header attributes */

	MGD77_nc_status (nc_inq_unlimdim (F->nc_id, &F->nc_recid));		/* Get id of unlimited dimension */
	if (F->nc_recid == -1) {	/* We are in deep trouble */
		fprintf (stderr, "%s: No record dimension in file %s - cannot read contents\n", GMT_program, file);
		return (MGD77_ERROR_NOT_MGD77PLUS);
	}
	MGD77_nc_status (nc_inq_dimname (F->nc_id, F->nc_recid, name));	/* Get dimension name */
	H->no_time = (strcmp (name, "time"));				/* True if data set has no time column */
	MGD77_nc_status (nc_inq_dimlen (F->nc_id, F->nc_recid, count));	/* Get number of records */
	H->n_records = count[0];

	/* Get information of all columns and store in header structure */
	
	nc_inq_nvars (F->nc_id, &n_vars);			/* Total number of variables in this file */
	for (id = c_id[MGD77_M77_SET] = c_id[MGD77_CDF_SET] = 0; id < n_vars && c_id[MGD77_M77_SET] < MGD77_SET_COLS && c_id[MGD77_CDF_SET] < MGD77_SET_COLS; id++) {	/* Keep checking for extra columns until all are found */
		
		MGD77_nc_status (nc_inq_varname    (F->nc_id, id, name));	/* Get column abbreviation */
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
		if (nc_get_att_double (F->nc_id, id, "scale_factor", &H->info[c].col[c_id[c]].scale)  == NC_ENOTATT) H->info[c].col[c_id[c]].scale = 1.0;	/* Get scale for reading */
		if (nc_get_att_double (F->nc_id, id, "add_offset",   &H->info[c].col[c_id[c]].offset) == NC_ENOTATT) H->info[c].col[c_id[c]].offset = 0.0;	/* Get offset for reading */

		/* In addition to scale_factor/offset, which are used to temporarily scale data to fit in the given nc_type format,
		 * it may have been discovered that the stored data are in the wrong unit (e.g., fathoms instead of meters, mGal instead of 0.1 mGal).
		 * Two optional terms, corr_factor and corr_offset, if present, are used to correct such mistakes (since original data wont be changed).
		 */
		if (nc_get_att_double (F->nc_id, id, "corr_factor",   &corr_factor) == NC_ENOTATT) corr_factor = 1.0;
		if (nc_get_att_double (F->nc_id, id, "corr_factor",   &corr_offset) == NC_ENOTATT) corr_offset = 0.0;
		if (F->use_corrections[c]) {	/* TRUE by default, but this can be turned off by changing this parameter in F */
			H->info[c].col[c_id[c]].scale *= corr_factor;	/* Combine effect of main and 2nd scale factors */
			H->info[c].col[c_id[c]].offset = H->info[c].col[c_id[c]].offset * corr_factor + corr_offset;	/* Combine effect of 2nd scale and 2nd offset into one offset */
		}
		H->info[c].col[c_id[c]].var_id = id;
		MGD77_nc_status (nc_inq_varndims (F->nc_id, id, &n_dims));	/* Get number of dimensions */
		MGD77_nc_status (nc_inq_vardimid (F->nc_id, id, dims));		/* Get dimension ids of this variable */
		if (n_dims == 2) {	/* 2-D text array */
			MGD77_nc_status (nc_inq_dimlen (F->nc_id, dims[1], count));		/* Get length of each string */
			H->info[c].col[c_id[c]].text = count[1];
		}
		else {	/* 1-d array or a text string */
			if (n_dims == 0 || dims[0] == F->nc_recid)	/* Scalar number or array of numbers */
				H->info[c].col[c_id[c]].text = 0;
			else {	/* Single text string, get its length */
				MGD77_nc_status (nc_inq_dimlen (F->nc_id, dims[0], count));	/* Get dimension length of this dimension */
				H->info[c].col[c_id[c]].text = count[0];
			}
		} 
		H->info[c].col[c_id[c]].constant = (n_dims == 0 || (n_dims == 1 && H->info[c].col[c_id[c]].text));	/* Field is constant for all records */
		H->info[c].col[c_id[c]].present = TRUE;	/* Field is present in this file */
		
		c_id[c]++;
	}
	for (c = 0; c < MGD77_N_SETS; c++) H->info[c].n_col = c_id[c];
	H->n_fields = H->info[MGD77_M77_SET].n_col + H->info[MGD77_CDF_SET].n_col;

	if ((err = MGD77_Order_Columns (F, H))) return (err);	/* Make sure requested columns are OK; if not given set defaults */
	
	return (MGD77_NO_ERROR); /* Success, unless failure */
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
	int i, k, c;
	
	if (F->n_out_columns) return;	/* Already made selection via MGD77_Select_Columns */
	
	/* Here, no selection is made, we return everything available in the file */
	
	for (c = k = 0; c < MGD77_N_SETS; c++) {
		for (i = 0; i < MGD77_SET_COLS; i++) {
			if (!H->info[c].col[i].present) continue;	/* This column is not available */
			F->order[k].set = c;
			F->order[k].item = i;
			H->info[c].col[i].pos = k;
			strcpy (F->desired_column[i], H->info[c].col[i].abbrev);
			k++;
		}
	}
	F->n_out_columns = k;
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
	int i, k, col, n_txt, n_val, id, err, n_nan_times, Clength[3] = {8, 5, 6};
	struct MGD77_DATA_RECORD MGD77Record;
	double *values[MGD77_N_NUMBER_FIELDS+1];
	char *text[3];
	
	for (k = n_txt = 0; k < F->n_out_columns; k++) if (S->H.info[MGD77_M77_SET].col[F->order[k].item].text) n_txt++;
	if (n_txt > 3) return (MGD77_ERROR_READ_ASC_DATA);
	
	for (k = 0; k < F->n_out_columns - n_txt; k++) values[k] = (double *)GMT_memory (VNULL, S->H.n_records, sizeof (double), "MGD77_Read_File_asc");
	for (k = 0; k < n_txt; k++) text[k] = (char *)GMT_memory (VNULL, S->H.n_records*Clength[k], sizeof (char), "MGD77_Read_File_asc");
	S->H.info[MGD77_M77_SET].bit_pattern = S->H.info[MGD77_CDF_SET].bit_pattern = 0;
	
	for (i = n_nan_times = 0; i < S->H.n_records; i++) {
		err = (F->format == MGD77_FORMAT_TBL) ? MGD77_Read_Data_Record_tbl (F, &MGD77Record) : MGD77_Read_Data_Record_m77 (F, &MGD77Record);
		if (err) return (err);
		for (col = n_txt = n_val = 0; col < F->n_out_columns; col++) {
			id = F->order[col].item;
			if (id >= MGD77_ID && id <= MGD77_SSPN) {
				k = id - MGD77_N_NUMBER_FIELDS;
				strncpy (&text[n_txt++][i*Clength[k]], MGD77Record.word[k], Clength[k]);
				
			}
			else
				values[n_val++][i] = (id == MGD77_TIME) ? MGD77Record.time : MGD77Record.number[id];
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

	err = MGD77_Write_Data_asc (file, F, S);	  /* Will write all MGD77 records in current file */
	if (err) return (err);

	err = MGD77_Close_File (F);
	if (err) return (err);

	return (MGD77_NO_ERROR);
}

int MGD77_Write_Data_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)	  /* Will write all MGD77 records in current file */
{
	int i, k, err, col[MGD77_N_DATA_FIELDS+1], id, Clength[3] = {8, 5, 6};
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
		for (k = 0, col[id] = MGD77_NOT_SET; k < F->n_out_columns; k++) if (!strcmp (S->H.info[MGD77_M77_SET].col[k].abbrev, mgd77defs[id].abbrev)) col[id] = k;
	}
	for (k = 0, col[MGD77_TIME] = MGD77_NOT_SET; k < F->n_out_columns; k++) if (!strcmp (S->H.info[MGD77_M77_SET].col[k].abbrev, "time")) col[MGD77_TIME] = k;
	make_ymdhm = (col[MGD77_TIME] >= 0 && (col[MGD77_YEAR] == MGD77_NOT_SET && col[MGD77_MONTH] == MGD77_NOT_SET && col[MGD77_DAY] == MGD77_NOT_SET && col[MGD77_HOUR] == MGD77_NOT_SET && col[MGD77_MIN] == MGD77_NOT_SET));
	
	memset ((void *)&MGD77Record, 0, sizeof (struct MGD77_DATA_RECORD));
	for (i = 0; i < S->H.n_records; i++) {
		MGD77Record.number[MGD77_RECTYPE] = (col[MGD77_RECTYPE] == MGD77_NOT_SET || GMT_is_dnan (values[col[MGD77_RECTYPE]][i])) ?  5.0 : values[col[MGD77_RECTYPE]][i];
		for (id = 1; id < MGD77_N_NUMBER_FIELDS; id++) {
			MGD77Record.number[id] = (col[id] >= 0) ? (double)values[col[id]][i] : GMT_d_NaN;
		}
		if (make_ymdhm) {	/* Split time into yyyy, mm, dd, hh, mm.xxx */
			MGD77Record.time = values[col[MGD77_TIME]][i];
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
				strncpy (MGD77Record.word[k], (char *)&text[col[id]][i*Clength[k]], Clength[k]);
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
		if (MGD77_Strip_Blanks) {	/* Remove leading and trailing blanks - may lead to empty string */
			k = strlen (currentField) - 1;
			while (k >= 0 && currentField[k] == ' ') k--;
			currentField[++k] = '\0';	/* No longer any trailing blanks */
			k = 0;
			while (currentField[k] && currentField[k] == ' ') k++;	/* Wind past any leading blanks */
			strcpy (MGD77Record->word[nwords], &currentField[k]);	/* Just copy text without changing it at all */
		}
		else
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
		MGD77Record->time = GMT_rdc2dt (rata_die, secs);	/* This gives GMT default abs time in seconds */
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

	if (!(fgets (line, BUFSIZ, F->fp))) return (MGD77_ERROR_READ_ASC_DATA);			/* Try to read one line from the file */

	GMT_chop (line);	/* Get rid of CR or LF */

	MGD77Record->bit_pattern = 0;
	for (i = pos = k = nwords = 0; i < MGD77_N_DATA_FIELDS; i++) {
		if (!GMT_strtok (line, ", \t", &pos, p)) return (MGD77_ERROR_READ_ASC_DATA);	/* Premature record end */
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
		MGD77Record->time = GMT_rdc2dt (rata_die, secs);	/* This gives GMT default abs time in seconds */
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
	 * Use MGD77_FORMAT_M77 to reset back to defaults (all OK) */
	 
	 if (format == MGD77_FORMAT_ANY) {
	 	MGD77_format_allowed[MGD77_FORMAT_M77] = TRUE;
	 	MGD77_format_allowed[MGD77_FORMAT_CDF] = TRUE;
	 	MGD77_format_allowed[MGD77_FORMAT_TBL] = TRUE;
	}
	else if (format >= MGD77_FORMAT_M77 && format <= MGD77_FORMAT_TBL)
		MGD77_format_allowed[format] = FALSE;
}

void MGD77_Init (struct MGD77_CONTROL *F, BOOLEAN remove_blanks)
{
	/* Initialize MGD77 control system */
	int i, t_index;
	struct passwd *pw;

	memset ((void *)F, 0, sizeof (struct MGD77_CONTROL));		/* Initialize structure */
	MGD77_Path_Init (F);
	MGD77_Init_Columns (F, NULL);
	F->use_flags[MGD77_M77_SET] = F->use_flags[MGD77_CDF_SET] = TRUE;		/* TRUE means programs will use error bitflags (if present) when returning data */
	F->use_corrections[MGD77_M77_SET] = F->use_corrections[MGD77_CDF_SET] = TRUE;	/* TRUE means we will apply correction factors (if present) when reading data */
	t_index = GMT_get_time_system ("unix");			/* Get index for GMT's Unix time */
	MGD77_Epoch_zero = GMT_time_system[t_index].epoch_t0;	/* Unix time's epoch t0 in GMT */
	memset ((void *)mgd77_range, 0, (size_t)(MGD77_N_DATA_EXTENDED * sizeof (struct MGD77_LIMITS)));
	for (i = 0; i < MGD77_SET_COLS; i++) MGD77_this_bit[i] = 1 << i;
	MGD77_Strip_Blanks = remove_blanks;
	gmtdefs.time_system = 4;	/* Use UNIX time as rtime */
	if ((pw = getpwuid (getuid ())) != NULL) {
		strcpy (F->user, pw->pw_name);
	}
	F->format = MGD77_FORMAT_ANY;
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
	int i, k, c, id, dummy;
	
	MGD77_Select_All_Columns (F, H);	/* Make sure n_out_columns is set */

	for (i = 0; i < F->n_out_columns; i++) {	/* This is not really needed if MGD77_Select_All_Columns did things, but just in case */
		if ((k = MGD77_Info_from_Abbrev (F->desired_column[i], H, &dummy)) == MGD77_NOT_SET) {
			fprintf (stderr, "%s: Requested column %s not in data set!\n", GMT_program, F->desired_column[i]);
			return (MGD77_ERROR_NOSUCHCOLUMN);
		}
		F->order[i].item = k % MGD77_SET_COLS;
		F->order[i].set  = k / MGD77_SET_COLS;
		H->info[F->order[i].set].col[F->order[i].item].pos = i;
	}
	
	for (i = 0; i < F->n_exact; i++) {	/* Determine column and info numbers from column name */
		F->Exact[i].col = MGD77_Get_Column (F->Exact[i].name, F);
	}
	
	/* F->Exact[] now holds F->n_exact values that refer to the output column order */
	
	for (i = 0; i < F->n_constraints; i++) {	/* Determine column and info numbers from column name */
		F->Constraint[i].col = MGD77_Get_Column (F->Constraint[i].name, F);
		c = F->order[F->Constraint[i].col].set;
		id = F->order[F->Constraint[i].col].item;
		if (H->info[c].col[id].text) {
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

int MGD77_Info_from_Abbrev (char *name, struct MGD77_HEADER *H, int *key)
{
	int i, c;
	
	/* Returns the number in the output list AND passes key as the entry in H */
	
	for (c = 0; c < MGD77_N_SETS; c++) {
		for (i = 0; i < H->info[c].n_col; i++) {
			if (!strcmp (name, H->info[c].col[i].abbrev)) {
				*key = H->info[c].col[i].pos;
				return (i + c * MGD77_SET_COLS);
			}
		}
	}
	*key = MGD77_NOT_SET;
	return (MGD77_NOT_SET);
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
		else if (!strcmp (word, "fdist")) {	/* Flat earth approximation (faster) */
			strcpy (word, "dist");
			F->flat_earth = TRUE;
		}
		else if (!strcmp (word, "edist")) {	/* Geodesic distances */
			strcpy (word, "dist");
			F->flat_earth = FALSE;
		}

		/* OK, here we are ready to update the structures */
		
		if (constraint) {	/* Got a column constraint, just key it by name for now */
			strcpy (F->Constraint[F->n_constraints].name, word);
			strcpy (F->Constraint[k].c_constraint, value);
			F->Constraint[k].code = constraint;
			F->Constraint[k].exact = exact;
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
			exit (EXIT_FAILURE);
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

	if ((this = getenv ("MGD77_HOME")) == CNULL) {
		if ((this = getenv ("GMTHOME")) != CNULL) {
			fprintf (stderr, "mgd77: Warning: MGD77_HOME not defined, set to $GMTHOME/share/mgd77\n");
			F->MGD77_HOME = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 13), 1, "MGD77_Set_Home");
			sprintf (F->MGD77_HOME, "%s/share/mgd77", this);
		}
		else {
			fprintf (stderr, "mgd77: ERROR: Neither MGD77_HOME or GMTHOME defined - give up\n");
			exit (EXIT_FAILURE);
		}
	}
	else {	/* Set default path */
		F->MGD77_HOME = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), 1, "MGD77_Set_Home");
		strcpy (F->MGD77_HOME, this);
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
			n_alloc += GMT_SMALL_CHUNK;
			F->MGD77_datadir = (char **) GMT_memory ((void *)F->MGD77_datadir, n_alloc, sizeof (char *), "MGD77_path_init");
		}
	}
	GMT_fclose (fp);
	F->MGD77_datadir = (char **) GMT_memory ((void *)F->MGD77_datadir, (size_t)F->n_MGD77_paths, sizeof (char *), "MGD77_path_init");
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
			exit (EXIT_FAILURE);
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

void MGD77_Set_Unit (char *dist, double *scale)
{	/* Return scale needed to convert a unit distance in the given unit to meter */
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
		
	exit (EXIT_FAILURE);
}

/* MGD77+ functions will be added down here */

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
	 
	int i, k, dims[2], Cdim_id[3], Clength[3] = {8, 5, 6}, var_id;
	time_t now;
	char *Cname[3] = {"id_dim", "sln_dim", "sspn_dim"}, string[128];
	BOOLEAN no_time;
	
	if (MGD77_Open_File (file, F, MGD77_WRITE_MODE)) return (-1);	/* Basically creates the full path */
	
	MGD77_nc_status (nc_create (F->path, NC_NOCLOBBER, &F->nc_id));	/* Create the file */
	
	/* Put attributes header, author, title and history */
	
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Conventions", strlen (MGD77_CDF_CONVENTION) + 1, (const char *)MGD77_CDF_CONVENTION));
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Version", strlen(MGD77_CDF_VERSION), (const char *)MGD77_CDF_VERSION));
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Author", strlen (H->author), H->author));
	sprintf (string, "Cruise %s (NGDC ID %s)", H->mgd77->Survey_Identifier, F->NGDC_id);
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "title", strlen (string), string));
	if (!H->history) {	/* Blank history, set initial message */
		(void) time (&now);
		sprintf (string, "%s Conversion from MGD77 ASCII to MGD77+ netCDF format", ctime(&now));
		k = strlen (string);
		for (i = 0; i < k; i++) if (string[i] == '\n') string[i] = ' ';	/* Remove the \n returned by ctime() */
		string[k++] = '\n';	string[k] = '\0';	/* Add LF at end of line */
		H->history = (char *)GMT_memory (VNULL, k, sizeof (char), GMT_program);
		strcpy (H->history, string);
	}
	/* else, history already filled out, use as is */
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "history", strlen (H->history), H->history));
	MGD77_Write_Header_Params (F, H->mgd77);	/* Write all the MGD77 header attributes */
	
	no_time = !(H->info[MGD77_M77_SET].bit_pattern & MGD77_this_bit[MGD77_TIME]);	/* Some cruises do not have time */
	
	if (no_time)
		MGD77_nc_status (nc_def_dim (F->nc_id, "record_no", NC_UNLIMITED, &F->nc_recid));	/* Define unlimited record dimension */
	else
		MGD77_nc_status (nc_def_dim (F->nc_id, "time", NC_UNLIMITED, &F->nc_recid));	/* Define unlimited time dimension */
		
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {			/* Loop over the 3 MGD77 text fields */
		k = i - MGD77_N_NUMBER_FIELDS;
		MGD77_nc_status (nc_def_dim (F->nc_id, Cname[k], Clength[k], &Cdim_id[k]));	/* Define character length dimension */
	}

	dims[0] = F->nc_recid;	dims[1] = 0;
	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Loop over all MGD77 number fields */
		if (i >= MGD77_YEAR && i <= MGD77_MIN) continue;	/* The 5 time-related columns are not written separately but as MGD77_TIME */
		if (! (H->info[MGD77_M77_SET].bit_pattern & MGD77_this_bit[i])) {
			if (gmtdefs.verbose == 2) fprintf (stderr, "%s: Field %s in data set %s are all NaN.  One value stored\n", GMT_program, mgd77defs[i].abbrev, file);
		}
		if (H->info[MGD77_M77_SET].col[i].constant)	/* Simply store one value */
			MGD77_nc_status (nc_def_var (F->nc_id, mgd77defs[i].abbrev, mgd77cdf[i].type, 0, NULL, &var_id));	/* Define a variable */
		else	/* Must store array */
			MGD77_nc_status (nc_def_var (F->nc_id, mgd77defs[i].abbrev, mgd77cdf[i].type, 1, dims, &var_id));	/* Define a variable */
		
		if (mgd77defs[i].fieldID[0] && strcmp (mgd77defs[i].fieldID, mgd77defs[i].abbrev)) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "long_name", strlen (mgd77defs[i].fieldID), mgd77defs[i].fieldID));
		if (mgd77cdf[i].units[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "units", strlen (mgd77cdf[i].units), mgd77cdf[i].units));
		if (!H->info[MGD77_M77_SET].col[i].constant) MGD77_nc_status (nc_put_att_double   (F->nc_id, var_id, "actual_range", NC_DOUBLE, 2, mgd77_range[i].limit));
		if (mgd77cdf[i].comment[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "comment", strlen (mgd77cdf[i].comment), mgd77cdf[i].comment));
		if (i == MGD77_DEPTH || i == MGD77_MSD) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "positive", 4, "down"));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "_FillValue", mgd77cdf[i].type, 1, &MGD77_NaN_val[mgd77cdf[i].type]));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "missing_value", mgd77cdf[i].type, 1, &MGD77_NaN_val[mgd77cdf[i].type]));
		if (mgd77cdf[i].scale  != 1.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "scale_factor", NC_DOUBLE, 1, &mgd77cdf[i].scale));
		if (mgd77cdf[i].offset != 0.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "add_offset", NC_DOUBLE, 1, &mgd77cdf[i].offset));
		H->info[MGD77_M77_SET].col[i].var_id = var_id;
	}
	
	/* Do absolute time separately */
		
	if (no_time) {
		if (gmtdefs.verbose) fprintf (stderr, "%s: Data set %s has no time values\n", GMT_program, file);
	}
	else {	/* We do have time, store them */
		MGD77_nc_status (nc_def_var        (F->nc_id, "time", NC_DOUBLE, 1, dims, &var_id));	/* Define a variable */
		MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "long_name", 4, "Time"));
		if (mgd77cdf[MGD77_TIME].units[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "units", strlen (mgd77cdf[MGD77_TIME].units), mgd77cdf[MGD77_TIME].units));
		MGD77_nc_status (nc_put_att_double   (F->nc_id, var_id, "actual_range", NC_DOUBLE, 2, mgd77_range[MGD77_TIME].limit));
		if (mgd77cdf[MGD77_TIME].comment[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "comment", strlen (mgd77cdf[MGD77_TIME].comment), mgd77cdf[MGD77_TIME].comment));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "_FillValue", NC_DOUBLE, 1, &MGD77_NaN_val[mgd77cdf[MGD77_TIME].type]));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "missing_value", NC_DOUBLE, 1, &MGD77_NaN_val[mgd77cdf[MGD77_TIME].type]));
		if (mgd77cdf[MGD77_TIME].scale  != 1.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "scale_factor", NC_DOUBLE, 1, &mgd77cdf[MGD77_TIME].scale));
		if (mgd77cdf[MGD77_TIME].offset != 0.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "add_offset", NC_DOUBLE, 1, &mgd77cdf[MGD77_TIME].offset));
		H->info[MGD77_M77_SET].col[MGD77_TIME].var_id = var_id;
	}
	
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {	/* Loop over the 3 MGD77 text fields */
		if (! (H->info[MGD77_M77_SET].bit_pattern & MGD77_this_bit[i])) {		/* No values for this data field */
			if (gmtdefs.verbose == 2) fprintf (stderr, "%s: Field %s in data set %s are all %s.  One value stored\n", GMT_program, mgd77defs[i].abbrev, file, mgd77defs[i].not_given);
		}		/* No values for this data field */
		k = i - MGD77_N_NUMBER_FIELDS;
		dims[1] = Cdim_id[k];
		if (H->info[MGD77_M77_SET].col[i].constant)	/* Simply store one value */
			MGD77_nc_status (nc_def_var (F->nc_id, mgd77defs[i].abbrev, mgd77cdf[i].type, 1, &dims[1], &var_id));	/* Define a variable */
		else	/* Must store array */
			MGD77_nc_status (nc_def_var (F->nc_id, mgd77defs[i].abbrev, mgd77cdf[i].type, 2, dims, &var_id));		/* Define a variable */
		/* MGD77_nc_status (nc_put_att_schar  (F->nc_id, var_id, "col_type", NC_BYTE, 1, &M)); */	/* Place attributes */
		if (mgd77defs[i].fieldID[0] && strcmp (mgd77defs[i].fieldID, mgd77defs[i].abbrev)) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "long_name", strlen (mgd77defs[i].fieldID), mgd77defs[i].fieldID));
		if (mgd77cdf[i].units[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "units", strlen (mgd77cdf[i].units), mgd77cdf[i].units));
		if (mgd77cdf[i].comment[0]) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "comment", strlen (mgd77cdf[i].comment), mgd77cdf[i].comment));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "_FillValue", mgd77cdf[i].type, 1, &MGD77_NaN_val[mgd77cdf[i].type]));
		MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "missing_value", mgd77cdf[i].type, 1, &MGD77_NaN_val[mgd77cdf[i].type]));
		H->info[MGD77_M77_SET].col[i].var_id = var_id;
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
	 
	int err, i, k, Clength[3] = {8, 5, 6};
	double *values;
	char *text;
	
	/* Determine if any fields are constant for all records */
	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Loop over all MGD77 number fields */
		if (i >= MGD77_YEAR && i <= MGD77_MIN) continue;		/* The 5 time-related columns are not written separately but as MGD77_TIME */
		values = (double *)S->values[i];
		/* Determine actual data range and if we need to store 1 or n values */
		S->H.info[MGD77_M77_SET].col[i].constant = (MGD77_dbl_are_constant (values, S->H.n_records, mgd77_range[i].limit));
	}
	values = (double *)S->values[MGD77_TIME];
	S->H.info[MGD77_M77_SET].col[MGD77_TIME].constant = (MGD77_dbl_are_constant (values, S->H.n_records, mgd77_range[MGD77_TIME].limit));
	mgd77_range[MGD77_TIME].limit[0] -= MGD77_Epoch_zero;	/* Make UNIX time for storage */ 
	mgd77_range[MGD77_TIME].limit[1] -= MGD77_Epoch_zero;	/* Make UNIX time for storage */ 
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {	/* Loop over the 3 MGD77 text fields */
		k = i - MGD77_N_NUMBER_FIELDS;
		text = (char *)S->values[i];
		S->H.info[MGD77_M77_SET].col[i].constant = (MGD77_txt_are_constant (text, S->H.n_records, Clength[k]));	/* Do we need to store 1 or n strings? */
	}

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
	 
	int i, k, n_bad = 0, Clength[3] = {8, 5, 6};
	size_t start[2] = {0, 0}, count[2] = {0, 0};
	double *values, *x, *xtmp = NULL, single_val;
	char *text;
	BOOLEAN transform, not_allocated = TRUE;
	
	count[0] = S->H.n_records;	count[1] = 0;
	
	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Loop over all MGD77 number fields (except Rec-type)*/
		if (i >= MGD77_YEAR && i <= MGD77_MIN) continue;	/* The 5 time-related columns are not written separately but as MGD77_TIME */
		transform = (! (mgd77cdf[i].scale == 1.0 && mgd77cdf[i].offset == 0.0));	/* TRUE if we must transform before writing */
		values = (double *)S->values[i];						/* Pointer to current double array */
		if (S->H.info[MGD77_M77_SET].col[i].constant) {	/* Only write a single value (after possibly transforming) */
			n_bad = MGD77_do_scale_offset_before_write (&single_val, values, 1, mgd77cdf[i].scale, mgd77cdf[i].offset, mgd77cdf[i].type);
			MGD77_nc_status (nc_put_var1_double (F->nc_id, S->H.info[MGD77_M77_SET].col[i].var_id, start, &single_val));
		}
		else {	/* Must write the entire array */
			if (transform) {	/* Must use temprary storage for scalings so that original values in S->values remain unchanged */
				if (not_allocated) xtmp = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Write_Data_cdf");	/* Get mem the first time */
				not_allocated = FALSE;	/* No longer the first time */
				n_bad = MGD77_do_scale_offset_before_write (xtmp, values, S->H.n_records, mgd77cdf[i].scale, mgd77cdf[i].offset, mgd77cdf[i].type);	/* mod copy */
				x = xtmp;	/* Points to modified copy */
			}
			else {	/* Save as is */
				x = values;	/* Points to original values */
				n_bad = 0;
			}
			MGD77_nc_status (nc_put_vara_double (F->nc_id, S->H.info[MGD77_M77_SET].col[i].var_id, start, count, x));
		}
		if (n_bad) {
			fprintf (stderr, "%s: %s [%s] had %d values outside valid range <%g,%g> for the chosen type (set to NaN = %g)\n",
				GMT_program, F->NGDC_id, S->H.info[MGD77_M77_SET].col[i].abbrev, n_bad, MGD77_Low_val[mgd77cdf[i].type],
				MGD77_High_val[mgd77cdf[i].type], MGD77_NaN_val[mgd77cdf[i].type]);
		}
	}
	
	/* Time: Store in Unix seconds since 1970 so we supply the required offset here directly */
	if (!S->H.info[MGD77_M77_SET].col[MGD77_TIME].constant) {	/* Time is available */
		values = (double *)S->values[MGD77_TIME];
		if (not_allocated) xtmp = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Write_Data_cdf");	/* Get mem the first time */
		not_allocated = FALSE;	/* No longer the first time for sure */
		MGD77_do_scale_offset_before_write (xtmp, values, S->H.n_records, 1.0, MGD77_Epoch_zero, mgd77cdf[MGD77_TIME].type);
		MGD77_nc_status (nc_put_vara_double (F->nc_id, S->H.info[MGD77_M77_SET].col[MGD77_TIME].var_id, start, count, xtmp));
	}

	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {	/* Loop over the 3 MGD77 text fields */
		k = i - MGD77_N_NUMBER_FIELDS;
		count[1] = Clength[k];
		text = (char *)S->values[i];
		if (S->H.info[MGD77_M77_SET].col[i].constant)
			MGD77_nc_status (nc_put_vara_schar (F->nc_id, S->H.info[MGD77_M77_SET].col[i].var_id, start, &count[1], (signed char *)text));	/* Just write one text string */
		else
			MGD77_nc_status (nc_put_vara_schar (F->nc_id, S->H.info[MGD77_M77_SET].col[i].var_id, start, count, (signed char *)text));
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
	for (i = 0; i < F->n_out_columns; i++) {
		c  = F->order[i].set;
		id = F->order[i].item;
		S->H.info[c].bit_pattern |= MGD77_this_bit[id];		/* We return this data field */
		if (!strcmp (S->H.info[c].col[id].abbrev, "time")) {	/* The time variable, select conversion from Unix time to GMT epoch time */
			scale = 1.0;
			offset = MGD77_Epoch_zero;
		}
		else {	/* Use the attribute scale & offset */
			scale = S->H.info[c].col[id].scale;
			offset = S->H.info[c].col[id].offset;
		}
		if (S->H.info[c].col[id].text) {	/* Text variable */
			count[1] = S->H.info[c].col[id].text;	/* Get length of each string */
			text = (char *) GMT_memory (VNULL, count[0] * count[1], sizeof (char), "MGD77_Read_File_cdf");
			if (S->H.info[c].col[id].constant) {	/* Scalar, must read one and then replicate */
				MGD77_nc_status (nc_get_vara_schar (F->nc_id, S->H.info[c].col[id].var_id, start, &count[1], (signed char *)text));
				for (k = 1; k < count[0]; k++) strncpy (&text[k*count[1]], text, count[1]);
			}
			else
				MGD77_nc_status (nc_get_vara_schar (F->nc_id, S->H.info[c].col[id].var_id, start, count, (signed char *)text));
			S->values[i] = (void *)text;
		}
		else {
			values = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Read_File_cdf");
			if (S->H.info[c].col[id].constant) {	/* Scalar, must read one and then replicate */
				MGD77_nc_status (nc_get_var1_double (F->nc_id, S->H.info[c].col[id].var_id, start, values));
				MGD77_do_scale_offset_after_read (values, 1, scale, offset, MGD77_NaN_val[S->H.info[c].col[id].type]);	/* Just modify one point */
				for (k = 1; k < count[0]; k++) values[k] = values[0];
			}
			else {	/* Read entire array */
				MGD77_nc_status (nc_get_vara_double (F->nc_id, S->H.info[c].col[id].var_id, start, count, values));
				MGD77_do_scale_offset_after_read (values, count[0], scale, offset, MGD77_NaN_val[S->H.info[c].col[id].type]);
			}
			S->values[i] = (void *)values;
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
			MGD77_do_scale_offset_after_read (&dvals[n_val], 1, H->info[c].col[id].scale, H->info[c].col[id].offset, MGD77_NaN_val[H->info[c].col[id].type]);
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
			MGD77_do_scale_offset_before_write (&single_val, &single_val, 1, H->info[c].col[id].scale, H->info[c].col[id].offset, H->info[c].col[id].type);
			MGD77_nc_status (nc_put_var1_double (F->nc_id, H->info[c].col[id].var_id, &start, &single_val));
		}
	}
	return (MGD77_NO_ERROR);
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
		exit (EXIT_FAILURE);
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
	char buffer [BUFSIZ], *SHAREDIR;
	int  i;

	memset ((void *)C, 0, sizeof (struct MGD77_CARTER));
	
	/* Read the correction table:  */

	if ((SHAREDIR = getenv ("GMTHOME")) == (char *)NULL) {
		fprintf (stderr, "MGD77_carter_init: Environment variable GMTHOME not set!\n");
                return (-1);
	}

	sprintf (buffer, "%s%cshare%cmgg%ccarter.d", SHAREDIR, DIR_DELIM, DIR_DELIM, DIR_DELIM);
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

	nominal_z1500 = 0.75 * twt_in_msec;

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

	part_in_100 = fmod (nominal_z1500, 100.0);

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

	guess = (depth_in_corr_m / 100.0) + min;
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
