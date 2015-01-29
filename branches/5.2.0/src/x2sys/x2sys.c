/*-----------------------------------------------------------------
 *	$Id$
 *
 *      Copyright (c) 1999-2015 by P. Wessel
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
/* x2sys.c contains the source code for the X2SYS crossover library
 * libx2sys.a.  The code is copylefted under the GNU Public Library
 * License.
 *
 * The following functions are external and user-callable form other
 * programs:
 *
 * x2sys_set_system	: Initialize X2SYS via the specified TAG
 * x2sys_initialize	: Reads the definition info file for current data files
 * x2sys_read_file	: Reads and returns the entire data matrix
 * x2sys_read_gmtfile	: Specifically reads an old .gmt file
 * x2sys_read_mgd77file : Specifically reads an ASCII MGD77 file
 * x2sys_read_mgd77ncfile : Specifically reads an netCDF MGD77+ file
 * x2sys_read_ncfile	: Specifically reads an COARDS netCDF file
 * x2sys_read_list	: Read an ascii list of track names
 * x2sys_dummytimes	: Make dummy times for tracks missing times
 * x2sys_n_data_cols	: Gives number of data columns in this data set
 * x2sys_fopen		: Opening files with error message and exit
 * x2sys_fclose		: Closes files and gives error messages
 * x2sys_skip_header	: Skips the header record(s) in the open file
 * x2sys_read_record	: Reads and returns one record from the open file
 * x2sys_pick_fields	: Decodes the -F<fields> flag of desired columns
 * x2sys_free_info	: Frees the information structure
 * x2sys_free_data	: Frees the data matrix
 * x2sys_read_coe_dbase : Reads into memory the entire COE ascii database
 * x2sys_free_coe_dbase : Free the array of COE structures
 *------------------------------------------------------------------
 * Core crossover functions are part of GMT:
 * GMT_init_track	: Prepares a track for crossover analysis
 * GMT_crossover	: Calculates crossovers for two data sets
 * GMT_x_alloc		: Allocate space for crossovers
 * GMT_x_free		: Free crossover structure
 * GMT_ysort		: Sorting routine used in x2sys_init_track [Hidden]
 *------------------------------------------------------------------
 * These routines are local to x2sys and used by the above routines:
 *
 * x2sys_set_home	: Initializes X2SYS paths
 * x2sys_record_length	: Returns the record length of current file
 *
 *------------------------------------------------------------------
 * Author:	Paul Wessel
 * Date:	20-SEP-2008
 * Version:	1.1, based on the spirit of the old xsystem code
 *
 */

/*
 *--------------------------------------------------------------------------------
 * Overview of how x2sys deals with data columns and selects those to use:
 *
 * The information of what data columns exist in a data set is maintained
 * in the <tag>.def file set up during x2sys_init.  So all x2sys programs
 * reading data knows this information, which is stored in an X2SYS_INFO
 * structure; call it s here.  To keep things clear, let icol refer to one
 * of the incoming (data file) columns while ocol refers to one of the
 * chosen outgoing (e.g., printed output) columns,  Thus,
 * 	s->n_fields		Total number of in-columns in file
 * In addition, a few special data columns are marked via column-variables:
 *	s->t_col		icol with time (or -1 if N/A)
 *	s->x_col		icol with longitude|x (or -1 if N/A)
 *	s->y_col		icol with latitude|y (or -1 if N/A)
 * Of course, these columns start at 0 for first column [C-counting].
 * Next, each column has an info struct, i.e., s->info[icol], in which
 * information about that column is kept.  Some examples:
 *	s->info[icol].name	text string with column name, e.g., "depth".
 *	s->info[icol].scale	factor to scale data, etc., most likely 1.
 * Now, many programs just want to extract a subset of all these columns and
 * the output order can be anything desired.  This selection is done by giving
 * an option like -Flon,lat,depth,faa
 * This string is parsed by x2sys_pick_fields which determines several things:
 *	s->n_out_cols		Number of items we selected (here 4)
 * It also fills out three important arrays allocated to length s->n_fields:
 *	s->use_column[icol]	true if column <icol> was selected for output
 *	s->out_order[ocol]	Refers to which icol goes with each ocol
 *				e.g., if time is the 4th input column (icol = 3)
 *				and we gave -Ftime,lat then s->out_order[0] = 3.
 * By looping ocol over s->out_order we find which icol to output.
 *	s->in_order[icol]	Refers to which ocol goes with each icol;
 *				this is the inverse of out_order.
 *				So if we want to know what ocol will be used
 *				for a given icol we look at s->in_order[icol]
 *				in colsultation with use_column[icol] since if
 *				that is false then that columns was not requested.
 * In case -F is not given then s->n_out_cols = s->n_fields, in|out_order are
 * initialized to 0,1,2,... and use_column[] = true so that by default all columns
 * are output in the same order as stored in the file.
 *--------------------------------------------------------------------------------
 */

#include "x2sys.h"
#include "gmt_internals.h"
#include "common_byteswap.h"

/* Global variables used by X2SYS functions */

char *X2SYS_HOME;
char *X2SYS_program;

struct MGD77_CONTROL M;

#define MAX_DATA_PATHS 32
char *x2sys_datadir[MAX_DATA_PATHS];	/* Directories where track data may live */
unsigned int n_x2sys_paths = 0;	/* Number of these directories */

/* Here are legacy functions for old GMT MGG supplement needed in x2sys */

char *gmtmgg_path[10];  /* Max 10 directories for now */
int n_gmtmgg_paths = 0; /* Number of these directories */

int gmtmggpath_func (char *leg_path, char *leg)
{
	int id;
	char geo_path[GMT_BUFSIZ] = {""};

	/* First look in current directory */

	sprintf (geo_path, "%s.gmt", leg);
	if (!access (geo_path, R_OK)) {
		strcpy (leg_path, geo_path);
		return (0);
	}

	/* Then look elsewhere */

	for (id = 0; id < n_gmtmgg_paths; id++) {
		sprintf (geo_path, "%s/%s.gmt", gmtmgg_path[id], leg);
		if (!access (geo_path, R_OK)) {
			strcpy (leg_path, geo_path);
			return (0);
		}
	}
	return(1);
}

/* gmtmggpath_init reads the GMT_SHAREDIR/mgg/gmtfile_paths or ~/.gmt/gmtfile_paths file and gets all
 * the gmtfile directories.
 */

void gmtmggpath_init (struct GMT_CTRL *GMT) {
	char line[GMT_BUFSIZ] = {""};
	FILE *fp = NULL;

	GMT_getsharepath (GMT, "mgg", "gmtfile_paths", "", line, R_OK);

	n_gmtmgg_paths = 0;

	if ((fp = fopen (line, "r")) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: path file %s for *.gmt files not found\n", line);
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "(Will only look in current directory for such files)\n");
		return;
	}

	while (fgets (line, GMT_BUFSIZ, fp)) {
		if (line[0] == '#') continue;	/* Comments */
		if (line[0] == ' ' || line[0] == '\0') continue;	/* Blank line */
		gmtmgg_path[n_gmtmgg_paths] = GMT_memory (GMT, NULL, strlen (line), char);
		line[strlen (line)-1] = 0;
		strcpy (gmtmgg_path[n_gmtmgg_paths], line);
		n_gmtmgg_paths++;
	}
	fclose (fp);
}

int get_first_year (struct GMT_CTRL *GMT, double t)
{
	/* obtain yyyy/mm/dd and return year */
	int64_t rd;
	double s;
	struct GMT_GCAL CAL;
	GMT_dt2rdc (GMT, t, &rd, &s);
	GMT_gcal_from_rd (GMT, rd, &CAL);
	return (CAL.year);
}

void x2sys_set_home (struct GMT_CTRL *GMT)
{
	char *this = NULL;
#ifdef WIN32
	static char *par = "%X2SYS_HOME%";
#else
	static char *par = "$X2SYS_HOME";
#endif

	if (X2SYS_HOME) return;	/* Already set elsewhere */

	if ((this = getenv ("X2SYS_HOME")) != NULL) {	/* Set user's default path */
		X2SYS_HOME = GMT_memory (GMT, NULL, strlen (this) + 1, char);
		strcpy (X2SYS_HOME, this);
	}
	else {	/* Require user to set this parameters since subdirs will be created and it would be messy to just use . */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: %s has not been set but is a required parameter\n", par);
		exit (EXIT_FAILURE);
	}
#ifdef WIN32
		DOS_path_fix (X2SYS_HOME);
#endif
}

void x2sys_path (struct GMT_CTRL *GMT_UNUSED(GMT), char *fname, char *path)
{
	sprintf (path, "%s/%s", X2SYS_HOME, fname);
}

FILE *x2sys_fopen (struct GMT_CTRL *GMT, char *fname, char *mode)
{
	FILE *fp = NULL;
	char file[GMT_BUFSIZ] = {""};

	if (mode[0] == 'w') {	/* Writing: Do this only in X2SYS_HOME */
		x2sys_path (GMT, fname, file);
		fp = fopen (file, mode);
	}
	else {			/* Reading: Try both current directory and X2SYS_HOME */
		if ((fp = fopen (fname, mode)) == NULL) {	/* Not in current directory, try $X2SYS_HOME */
			x2sys_path (GMT, fname, file);
			fp = fopen (file, mode);
		}
	}
	return (fp);
}

int x2sys_access (struct GMT_CTRL *GMT, char *fname, int mode)
{
	int k;
	char file[GMT_BUFSIZ] = {""};
	x2sys_path (GMT, fname, file);
	if ((k = access (file, mode))) {	/* Not in X2SYS_HOME directory */
		k = access (fname, mode);	/* Try in current directory */
	}
	return (k);
}

int x2sys_fclose (struct GMT_CTRL *GMT_UNUSED(GMT), char *GMT_UNUSED(fname), FILE *fp)
{

	if (fclose (fp)) return (X2SYS_FCLOSE_ERR);
	return (X2SYS_NOERROR);
}

void x2sys_skip_header (struct GMT_CTRL *GMT, FILE *fp, struct X2SYS_INFO *s)
{
	unsigned int i;
	char line[GMT_BUFSIZ] = {""};

	if (s->file_type == X2SYS_ASCII) {	/* ASCII, skip records */
		for (i = 0; i < s->skip; i++) {
			if (!fgets (line, GMT_BUFSIZ, fp)) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error in header line %d\n", i);
				exit (EXIT_FAILURE);
			}
		}
	}
	else if (s->file_type == X2SYS_BINARY) {			/* Native binary, skip bytes */
		if (fseek (fp, (off_t)s->skip, SEEK_CUR)) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Seed error while skipping headers\n");
			exit (EXIT_FAILURE);
		}
	}
}

int x2sys_initialize (struct GMT_CTRL *GMT, char *TAG, char *fname, struct GMT_IO *G,  struct X2SYS_INFO **I)
{
	/* Reads the format definition file and sets all information variables */

	unsigned int i = 0;
	int is;
	size_t n_alloc = GMT_TINY_CHUNK;
	int c;	/* Remain 4-byte integer */
	FILE *fp = NULL;
	struct X2SYS_INFO *X = NULL;
	char line[GMT_BUFSIZ] = {""}, cardcol[80] = {""}, yes_no;

	x2sys_set_home (GMT);

	X = GMT_memory (GMT, NULL, n_alloc, struct X2SYS_INFO);
	X->TAG = strdup (TAG);
	X->info = GMT_memory (GMT, NULL, n_alloc, struct X2SYS_DATA_INFO);
	X->file_type = X2SYS_ASCII;
	X->x_col = X->y_col = X->t_col = -1;
	X->ms_flag = '>';	/* Default multisegment header flag */
	sprintf (line, "%s/%s.def", TAG, fname);
	X->dist_flag = 0;	/* Cartesian distances */
	sprintf (X->separators, "%s\n", GMT_TOKEN_SEPARATORS);

	if ((fp = x2sys_fopen (GMT, line, "r")) == NULL) return (X2SYS_BAD_DEF);

	X->unit[X2SYS_DIST_SELECTION][0] = 'k';		X->unit[X2SYS_DIST_SELECTION][1] = '\0';	/* Initialize for geographic data (km and m/s) */
	X->unit[X2SYS_SPEED_SELECTION][0] = GMT_MAP_DIST_UNIT;	X->unit[X2SYS_SPEED_SELECTION][1] = '\0';
	if (!strcmp (fname, "mgd77+")) {
		X->read_file = &x2sys_read_mgd77ncfile;
		X->geographic = true;
		X->geodetic = GMT_IS_0_TO_P360_RANGE;
		X->dist_flag = 2;	/* Creat circle distances */
		MGD77_Init (GMT, &M);	/* Initialize MGD77 Machinery */
	}
	else if (!strcmp (fname, "gmt") && GMT_compat_check (GMT, 4)) {
		X->read_file = &x2sys_read_gmtfile;
		X->geographic = true;
		X->geodetic = GMT_IS_0_TO_P360_RANGE;
		X->dist_flag = 2;	/* Creat circle distances */
	}
	else if (!strcmp (fname, "mgd77")) {
		X->read_file = &x2sys_read_mgd77file;
		X->geographic = true;
		X->geodetic = GMT_IS_0_TO_P360_RANGE;
		X->dist_flag = 2;	/* Creat circle distances */
		MGD77_Init (GMT, &M);	/* Initialize MGD77 Machinery */
	}
	else {
		X->read_file = &x2sys_read_file;
		X->dist_flag = 0;			/* Cartesian distances */
		X->unit[X2SYS_DIST_SELECTION][0] = 'c';	/* Reset to Cartesian */
		X->unit[X2SYS_SPEED_SELECTION][0] = 'c';	/* Reset to Cartesian */
	}
	while (fgets (line, GMT_BUFSIZ, fp)) {
		if (line[0] == '\0') continue;
		if (line[0] == '#') {
			if (!strncmp (line, "#SKIP",   5U)) X->skip = atoi (&line[6]);
			if (!strncmp (line, "#ASCII",  6U)) X->file_type = X2SYS_ASCII;
			if (!strncmp (line, "#BINARY", 7U)) X->file_type = X2SYS_BINARY;
			if (!strncmp (line, "#NETCDF", 7U)) X->file_type = X2SYS_NETCDF;
			if (!strncmp (line, "#GEO", 4U)) X->geographic = true;
			if (!strncmp (line, "#MULTISEG", 9U)) {
				X->multi_segment = true;
				sscanf (line, "%*s %c", &X->ms_flag);
			}
			continue;
		}
		GMT_chop (line);	/* Remove trailing CR or LF */

		sscanf (line, "%s %c %c %lf %lf %lf %s %s", X->info[i].name, &X->info[i].intype, &yes_no, &X->info[i].nan_proxy, &X->info[i].scale, &X->info[i].offset, X->info[i].format, cardcol);
		if (X->info[i].intype == 'A') {	/* ASCII Card format */
			sscanf (cardcol, "%d-%d", &X->info[i].start_col, &X->info[i].stop_col);
			X->info[i].n_cols = X->info[i].stop_col - X->info[i].start_col + 1;
		}
		c = X->info[i].intype;
		if (tolower (c) == 'a') X->file_type = X2SYS_ASCII;
		c = yes_no;
		if (tolower (c) != 'Y') X->info[i].has_nan_proxy = true;
		if (!(X->info[i].scale == 1.0 && X->info[i].offset == 0.0)) X->info[i].do_scale = true;
		if (!strcmp (X->info[i].name, "x") || !strcmp (X->info[i].name, "lon"))  X->x_col = i;
		if (!strcmp (X->info[i].name, "y") || !strcmp (X->info[i].name, "lat"))  X->y_col = i;
		if (!strcmp (X->info[i].name, "t") || !strcmp (X->info[i].name, "time")) X->t_col = i;
		i++;
		if (i == n_alloc) {
			n_alloc <<= 1;
			X->info = GMT_memory (GMT, X->info, n_alloc, struct X2SYS_DATA_INFO);
		}

	}
	fclose (fp);
	if (X->file_type == X2SYS_NETCDF) X->read_file = &x2sys_read_ncfile;

	if (i < n_alloc) X->info = GMT_memory (GMT, X->info, i, struct X2SYS_DATA_INFO);
	X->n_fields = X->n_out_columns = i;

	if (X->file_type == X2SYS_BINARY) {	/* Binary mode needed */
		strcpy (G->r_mode, "rb");
		strcpy (G->w_mode, "wb");
		strcpy (G->a_mode, "ab+");
	}
	X->in_order   = GMT_memory (GMT, NULL, X->n_fields, unsigned int);
	X->out_order  = GMT_memory (GMT, NULL, X->n_fields, unsigned int);
	X->use_column = GMT_memory (GMT, NULL, X->n_fields, bool);
	for (i = is = 0; i < X->n_fields; i++, is++) {	/* Default is same order and use all columns */
		X->in_order[i] = X->out_order[i] = i;
		X->use_column[i] = 1;
		G->col_type[GMT_IN][i] = G->col_type[GMT_OUT][i] = (X->x_col == is) ? GMT_IS_LON : ((X->y_col == is) ? GMT_IS_LAT : GMT_IS_UNKNOWN);
		if (X->x_col == is) 
			G->col_type[GMT_IN][i] = G->col_type[GMT_OUT][i] = GMT_IS_LON;
		else if (X->y_col == is)
			G->col_type[GMT_IN][i] = G->col_type[GMT_OUT][i] = GMT_IS_LAT;
		else if (X->t_col == is)
			G->col_type[GMT_IN][i] = G->col_type[GMT_OUT][i] = GMT_IS_ABSTIME;
		else
			G->col_type[GMT_IN][i] = G->col_type[GMT_OUT][i] = GMT_IS_UNKNOWN;
	}
	X->n_data_cols = x2sys_n_data_cols (GMT, X);
	X->rec_size = (8 + X->n_data_cols) * sizeof (double);

	*I = X;
	return (X2SYS_NOERROR);
}

void x2sys_end (struct GMT_CTRL *GMT, struct X2SYS_INFO *X)
{	/* Free allcoated memory */
	unsigned int id;
	if (X2SYS_HOME) GMT_free (GMT, X2SYS_HOME);
	if (!X) return;
	if (X->in_order) GMT_free (GMT, X->in_order);
	if (X->out_order) GMT_free (GMT, X->out_order);
	if (X->use_column) GMT_free (GMT, X->use_column);
	free (X->TAG);	/* free since allocated by strdup */
	x2sys_free_info (GMT, X);
	for (id = 0; id < n_x2sys_paths; id++) GMT_free (GMT, x2sys_datadir[id]);
	MGD77_end (GMT, &M);
}

unsigned int x2sys_record_length (struct GMT_CTRL *GMT_UNUSED(GMT), struct X2SYS_INFO *s)
{
	unsigned int i, rec_length = 0;

	for (i = 0; i < s->n_fields; i++) {
		switch (s->info[i].intype) {
			case 'c':
			case 'u':
				rec_length += 1;
				break;
			case 'h':
				rec_length += 2;
				break;
			case 'i':
			case 'f':
				rec_length += 4;
				break;
			case 'l':
				rec_length += (unsigned int)sizeof (long);
				break;
			case 'd':
				rec_length += 8;
				break;
		}
	}
	return (rec_length);
}

unsigned int x2sys_n_data_cols (struct GMT_CTRL *GMT_UNUSED(GMT), struct X2SYS_INFO *s)
{
	unsigned int i, n = 0;
	int is;

	for (i = is = 0; i < s->n_out_columns; i++, is++) {	/* Loop over all possible fields in this data set */
		if (is == s->x_col) continue;
		if (is == s->y_col) continue;
		if (is == s->t_col) continue;
		n++;	/* Only count data columns */
	}

	return (n);
}

int x2sys_pick_fields (struct GMT_CTRL *GMT, char *string, struct X2SYS_INFO *s)
{
	/* Scan the -Fstring and select which columns to use and which order
	 * they should appear on output.  Default is all columns and the same
	 * order as on input.  Once this is set you can loop through i = 0:n_out_columns
	 * and use out_order[i] to get the original column number. Or you can loop
	 * over all in_order[i] to determine what output column they will be put in,
	 * provided use_column[i] is true.
	 */

	char line[GMT_BUFSIZ] = {""}, p[GMT_BUFSIZ] = {""};
	unsigned int i = 0, j, pos = 0;

	strncpy (s->fflags, string, GMT_BUFSIZ);
	strncpy (line, string, GMT_BUFSIZ);	/* Make copy for later use */
	GMT_memset (s->use_column, s->n_fields, bool);

	while ((GMT_strtok (line, ",", &pos, p))) {
		j = 0;
		while (j < s->n_fields && strcmp (p, s->info[j].name)) j++;
		if (j < s->n_fields) {
			s->out_order[i] = j;
			s->in_order[j]  = i;
			s->use_column[j] = true;
		}
		else {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "X2SYS: Error: Unknown column name %s\n", p);
			return (X2SYS_BAD_COL);
		}
		i++;
	}

	s->n_out_columns = i;

	return (X2SYS_NOERROR);
}

void x2sys_free_info (struct GMT_CTRL *GMT, struct X2SYS_INFO *s)
{
	GMT_free (GMT, s->info);
	GMT_free (GMT, s);
}

void x2sys_free_data (struct GMT_CTRL *GMT, double **data, unsigned int n, struct X2SYS_FILE_INFO *p)
{
	unsigned int i;

	for (i = 0; i < n; i++) {
		if (data[i]) GMT_free (GMT, data[i]);
	}
	GMT_free (GMT, data);
	if (p->ms_rec) GMT_free (GMT, p->ms_rec);
}

double *x2sys_dummytimes (struct GMT_CTRL *GMT, uint64_t n)
{
	uint64_t i;
	double *t;

	/* Make monotonically increasing dummy time sequence */

	t = GMT_memory (GMT, NULL, n, double);

	for (i = 0; i < n; i++) t[i] = (double)i;

	return (t);
}

/*
 * x2sys_data_read:  Read subroutine for x2_sys data input.
 * This function will read one logical record of ascii or
 * binary data from the open file, and return with a double
 * array called data[] with each data value in it.
 */

int x2sys_read_record (struct GMT_CTRL *GMT, FILE *fp, double *data, struct X2SYS_INFO *s, struct GMT_IO *G)
{
	bool error = false;
	unsigned int j, k, i, pos;
	int is;
	size_t n_read = 0;
	char line[GMT_BUFSIZ] = {""}, buffer[GMT_LEN64] = {""}, p[GMT_BUFSIZ] = {""}, c;
	unsigned char u;
	short int h;
	float f;
	long L;
	double NaN = GMT->session.d_NaN;

	for (j = 0; !error && j < s->n_fields; j++) {

		switch (s->info[j].intype) {

			case 'A':	/* ASCII Card Record, must extract columns */
				if (j == 0) {
					s->ms_next = false;
					if (!fgets (line, GMT_BUFSIZ, fp)) return (-1);
					while (line[0] == '#' || line[0] == s->ms_flag) {
						if (!fgets (line, GMT_BUFSIZ, fp)) return (-1);
						if (s->multi_segment) s->ms_next = true;
					}
					GMT_chop (line);	/* Remove trailing CR or LF */
				}
				strncpy (buffer, &line[s->info[j].start_col], s->info[j].n_cols);
				buffer[s->info[j].n_cols] = 0;
				if (GMT_scanf (GMT, buffer, G->col_type[GMT_IN][j], &data[j]) == GMT_IS_NAN) data[j] = GMT->session.d_NaN;
				break;

			case 'a':	/* ASCII Record, get all columns directly */
				k = 0;
				s->ms_next = false;
				if (!fgets (line, GMT_BUFSIZ, fp)) return (-1);
				while (line[0] == '#' || line[0] == s->ms_flag) {
					if (!fgets (line, GMT_BUFSIZ, fp)) return (-1);
					if (s->multi_segment) s->ms_next = true;
				}
				GMT_chop (line);	/* Remove trailing CR or LF */
				pos = 0;
				while ((GMT_strtok (line, s->separators, &pos, p)) && k < s->n_fields) {
					if (GMT_scanf (GMT, p, G->col_type[GMT_IN][k], &data[k]) == GMT_IS_NAN) data[k] = GMT->session.d_NaN;
					k++;;
				}
				return ((k != s->n_fields) ? -1 : 0);
				break;

			case 'c':	/* Binary signed 1-byte character */
				n_read += fread (&c, sizeof (char), 1U, fp);
				data[j] = (double)c;
				break;

			case 'u':	/* Binary unsigned 1-byte character */
				n_read += fread (&u, sizeof (unsigned char), 1U, fp);
				data[j] = (double)u;
				break;

			case 'h':	/* Binary signed 2-byte integer */
				n_read += fread (&h, sizeof (short int), 1U, fp);
				data[j] = (double)h;
				break;

			case 'i':	/* Binary signed 4-byte integer */
				n_read += fread (&i, sizeof (int), 1U, fp);
				data[j] = (double)i;
				break;

			case 'l':	/* Binary signed 4/8-byte integer (long) */
				n_read += fread (&L, sizeof (long), 1U, fp);
				data[j] = (double)L;
				break;

			case 'f':	/* Binary signed 4-byte float */
				n_read += fread (&f, sizeof (float), 1U, fp);
				data[j] = (double)f;
				break;

			case 'd':	/* Binary signed 8-byte float */
				n_read += fread (&data[j], sizeof (double), 1U, fp);
				break;

			default:
				error = true;
				break;
		}
	}

	/* Change nan-proxies to NaNs and apply any data scales and offsets */

	for (i = is = 0; i < s->n_fields; i++, is++) {
		if (s->info[i].has_nan_proxy && data[i] == s->info[i].nan_proxy)
			data[i] = NaN;
		else if (s->info[i].do_scale)
			data[i] = data[i] * s->info[i].scale + s->info[i].offset;
		if (GMT_is_dnan (data[i])) s->info[i].has_nans = true;
		if (is == s->x_col && s->geographic) GMT_lon_range_adjust (s->geodetic, &data[i]);
	}

	return ((error || n_read != s->n_fields) ? -1 : 0);
}

int x2sys_read_file (struct GMT_CTRL *GMT, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, uint64_t *n_rec)
{
	/* Reads the entire contents of the file given and returns the
	 * number of data records.  The data matrix is return in the
	 * pointer data.
	 */

	uint64_t j;
 	unsigned int i;
	bool first = true;
	size_t n_alloc;
	FILE *fp = NULL;
	double **z = NULL, *rec = NULL;
	char path[GMT_BUFSIZ] = {""};

	if (x2sys_get_data_path (GMT, path, fname, s->suffix)) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_file : Cannot find track %s\n", fname);
  		return (-1);
	}
	if ((fp = fopen (path, G->r_mode)) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_file : Cannot open file %s\n", path);
  		return (-1);
	}
	strcpy (s->path, path);

	n_alloc = GMT_CHUNK;

	rec = GMT_memory (GMT, NULL, s->n_fields, double);
	z = GMT_memory (GMT, NULL, s->n_fields, double *);
	for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, NULL, n_alloc, double);
	p->ms_rec = GMT_memory (GMT, NULL, n_alloc, uint64_t);
	x2sys_skip_header (GMT, fp, s);
	p->n_segments = 0;	/* So that first increment sets it to 0 */
	j = 0;
	while (!x2sys_read_record (GMT, fp, rec, s, G)) {	/* Gets the next data record */
		if (s->multi_segment && s->ms_next && !first) p->n_segments++;
		for (i = 0; i < s->n_fields; i++) z[i][j] = rec[i];
		p->ms_rec[j] = p->n_segments;
		j++;
		if (j == n_alloc) {	/* Get more */
			n_alloc <<= 1;
			for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, z[i], n_alloc, double);
			p->ms_rec = GMT_memory (GMT, p->ms_rec, n_alloc, uint64_t);
		}
		first = false;
	}
	p->n_segments++;	/* To get the total number of segments 0-(n_segments-1) */
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "x2sys_read_file : File %s contained %" PRIu64 " segments\n", path, p->n_segments);
	
	fclose (fp);
	GMT_free (GMT, rec);
	for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, z[i], j, double);
	p->ms_rec = GMT_memory (GMT, p->ms_rec, j, uint64_t);

	*data = z;

	p->n_rows = j;
	p->year = 0;
	strncpy (p->name, fname, 32U);
	*n_rec = p->n_rows;

	return (X2SYS_NOERROR);
}

int x2sys_read_gmtfile (struct GMT_CTRL *GMT, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, uint64_t *n_rec)
{
	/* Reads the entire contents of the file given and returns the
	 * number of data records.  The data matrix is return in the
	 * pointer data.  The input file format is the venerable GMT
	 * MGG format from old Lamont by Wessel and Smith.
	 */

	int i, year, n_records;	/* These must remain 4-byte ints */
	int64_t rata_day;
	uint64_t j;
	char path[GMT_BUFSIZ] = {""};
	FILE *fp = NULL;
	double **z = NULL;
	double NaN = GMT->session.d_NaN, t_off;
	struct GMTMGG_REC record;

 	if (n_x2sys_paths) {
  		if (x2sys_get_data_path (GMT, path, fname, s->suffix)) return (GMT_GRDIO_FILE_NOT_FOUND);
	}
	else {
		char name[80] = {""};
		if (!(s->flags & 1)) {	/* Must init gmt file paths */
			gmtmggpath_init (GMT);
			s->flags |= 1;
		}
		strncpy (name, fname, 80U);
		if (strstr (fname, ".gmt")) name[strlen(fname)-4] = 0;	/* Name includes .gmt suffix, remove it */
	  	if (gmtmggpath_func (path, name)) return (GMT_GRDIO_FILE_NOT_FOUND);

	}
	strcpy (s->path, path);
	if ((fp = fopen (path, G->r_mode)) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_file : Cannot open file %s\n", path);
  		return (-1);
	}

	if (fread (&year, sizeof (int), 1U, fp) != 1U) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_gmtfile: Could not read leg year from %s\n", path);
		return (-1);
	}
	p->year = year;
	rata_day = GMT_rd_from_gymd (GMT, year, 1, 1);	/* Get the rata day for start of cruise year */
	t_off = GMT_rdc2dt (GMT, rata_day, 0.0);		/* Secs to start of day */


	if (fread (&n_records, sizeof (int), 1U, fp) != 1U) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_gmtfile: Could not read n_records from %s\n", path);
		return (GMT_GRDIO_READ_FAILED);
	}
	p->n_rows = n_records;
	GMT_memset (p->name, 32, char);

	if (fread (p->name, sizeof (char), 10U, fp) != 10U) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_gmtfile: Could not read agency from %s\n", path);
		return (GMT_GRDIO_READ_FAILED);
	}

	z = GMT_memory (GMT, NULL, 6, double *);
	for (i = 0; i < 6; i++) z[i] = GMT_memory (GMT, NULL, p->n_rows, double);

	for (j = 0; j < p->n_rows; j++) {

		if (fread (&record, 18U, 1U, fp) != 1) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_gmtfile: Could not read record %" PRIu64 " from %s\n", j, path);
			return (GMT_GRDIO_READ_FAILED);
		}

		z[0][j] = record.time * GMT->current.setting.time_system.i_scale + t_off;	/* To get GMT time keeping */
		z[1][j] = record.lat * MDEG2DEG;
		z[2][j] = record.lon * MDEG2DEG;
		z[3][j] = (record.gmt[0] == GMTMGG_NODATA) ? NaN : 0.1 * record.gmt[0];
		z[4][j] = (record.gmt[1] == GMTMGG_NODATA) ? NaN : record.gmt[1];
		z[5][j] = (record.gmt[2] == GMTMGG_NODATA) ? NaN : record.gmt[2];

	}

	fclose (fp);

	p->ms_rec = NULL;
	p->n_segments = 0;

	*n_rec = p->n_rows;
	*data = z;

	return (X2SYS_NOERROR);
}

int x2sys_read_mgd77file (struct GMT_CTRL *GMT, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *GMT_UNUSED(G), uint64_t *n_rec)
{
	uint64_t i, j;
	size_t n_alloc = GMT_CHUNK;
	int col[MGD77_N_DATA_EXTENDED];
	char path[GMT_BUFSIZ] = {""}, *tvals[MGD77_N_STRING_FIELDS];
	double **z = NULL, dvals[MGD77_N_DATA_EXTENDED];
	struct MGD77_HEADER H;
	struct MGD77_CONTROL M;

	MGD77_Init (GMT, &M);	/* Initialize MGD77 Machinery */

  	if (n_x2sys_paths) {
  		if (x2sys_get_data_path (GMT, path, fname, s->suffix)) return (GMT_GRDIO_FILE_NOT_FOUND);
		if (MGD77_Open_File (GMT, path, &M, 0)) return (GMT_GRDIO_OPEN_FAILED);
	}
	else if (MGD77_Open_File (GMT, fname, &M, 0))
		return (GMT_GRDIO_FILE_NOT_FOUND);
	strcpy (s->path, M.path);

	if (MGD77_Read_Header_Record (GMT, fname, &M, &H)) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error reading header sequence for cruise %s\n", fname);
		return (GMT_GRDIO_READ_FAILED);
	}

	for (i = 0; i < MGD77_N_STRING_FIELDS; i++) tvals[i] = GMT_memory (GMT, NULL, 9, char);
	z = GMT_memory (GMT, NULL, s->n_fields, double *);
	for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, NULL, n_alloc, double);
	for (i = 0; i < s->n_out_columns; i++) {
		col[i] = MGD77_Get_Column (GMT, s->info[s->out_order[i]].name, &M);
	}

	p->year = 0;
	j = 0;
	while (!MGD77_Read_Data_Record (GMT, &M, &H, dvals, tvals)) {		/* While able to read a data record */
		GMT_lon_range_adjust (s->geodetic, &dvals[MGD77_LONGITUDE]);
		for (i = 0; i < s->n_out_columns; i++) z[i][j] = dvals[col[i]];
		if (p->year == 0 && !GMT_is_dnan (dvals[0])) p->year = get_first_year (GMT, dvals[0]);
		j++;
		if (j == n_alloc) {
			n_alloc <<= 1;
			for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, z[i], n_alloc, double);
		}
	}
	MGD77_Close_File (GMT, &M);
	MGD77_Free_Header_Record (GMT, &M, &H);	/* Free up header structure */
	MGD77_end (GMT, &M);

	strncpy (p->name, fname, 32U);
	p->n_rows = j;
	for (i = 0; i < s->n_fields; i++) z[i] = GMT_memory (GMT, z[i], p->n_rows, double);

	p->ms_rec = NULL;
	p->n_segments = 0;
	for (i = 0; i < MGD77_N_STRING_FIELDS; i++) GMT_free (GMT, tvals[i]);

	*data = z;
	*n_rec = p->n_rows;

	return (X2SYS_NOERROR);
}

int x2sys_read_mgd77ncfile (struct GMT_CTRL *GMT, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *GMT_UNUSED(G), uint64_t *n_rec)
{
	uint64_t i;
	char path[GMT_BUFSIZ] = {""};
	double **z = NULL;
	struct MGD77_DATASET *S = NULL;
	struct MGD77_CONTROL M;

	MGD77_Init (GMT, &M);			/* Initialize MGD77 Machinery */
	M.format  = MGD77_FORMAT_CDF;		/* Set input file's format to netCDF */
	MGD77_Select_Format (GMT, M.format);	/* Only allow the specified MGD77 input format */

	for (i = 0; i < s->n_out_columns; i++)
		M.desired_column[i] = strdup(s->info[s->out_order[i]].name);	/* Set all the required fields */
	M.n_out_columns = s->n_out_columns;

	S = MGD77_Create_Dataset (GMT);	/* Get data structure w/header */

  	if (n_x2sys_paths) {
  		if (x2sys_get_data_path (GMT, path, fname, s->suffix)) return (GMT_GRDIO_FILE_NOT_FOUND);
		if (MGD77_Open_File (GMT, path, &M, 0)) return (GMT_GRDIO_OPEN_FAILED);
	}
	else if (MGD77_Open_File (GMT, fname, &M, 0))
		return (GMT_GRDIO_FILE_NOT_FOUND);
	strcpy (s->path, M.path);

	if (MGD77_Read_Header_Record (GMT, fname, &M, &S->H)) {	/* Returns info on all columns */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_mgd77ncfile: Error reading header sequence for cruise %s\n", fname);
     		return (GMT_GRDIO_READ_FAILED);
	}

	if (MGD77_Read_Data (GMT, fname, &M, S)) {	/* Only gets the specified columns and barfs otherwise */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_mgd77ncfile: Error reading data set for cruise %s\n", fname);
     		return (GMT_GRDIO_READ_FAILED);
	}
	MGD77_Close_File (GMT, &M);

	z = GMT_memory (GMT, NULL, M.n_out_columns, double *);
	for (i = 0; i < M.n_out_columns; i++) z[i] = S->values[i];

	strncpy (p->name, fname, 32U);
	p->n_rows = S->H.n_records;
	p->ms_rec = NULL;
	p->n_segments = 0;
	p->year = S->H.meta.Departure[0];
	for (i = 0; i < MGD77_N_SETS; i++) if (S->flags[i]) GMT_free (GMT, S->flags[i]);
	MGD77_Free_Header_Record (GMT, &M, &(S->H));	/* Free up header structure */
	GMT_free (GMT, S);
	MGD77_end (GMT, &M);

	*data = z;
	*n_rec = p->n_rows;

	return (X2SYS_NOERROR);
}

int x2sys_read_ncfile (struct GMT_CTRL *GMT, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *GMT_UNUSED(G), uint64_t *n_rec)
{
	int n_fields, ns = s->n_out_columns;
	uint64_t n_expect = GMT_MAX_COLUMNS;
	uint64_t i, j;
	char path[GMT_BUFSIZ] = {""};
	double **z = NULL, *in = NULL;
	FILE *fp = NULL;

  	if (x2sys_get_data_path (GMT, path, fname, s->suffix)) return (GMT_GRDIO_FILE_NOT_FOUND);
	strcat (path, "?");	/* Set all the required fields */
	for (i = 0; i < s->n_out_columns; i++) {
		if (i) strcat (path, "/");
		strcat (path, s->info[s->out_order[i]].name);
	}

	strcpy (s->path, path);

	GMT_parse_common_options (GMT, "b", 'b', "c");	/* Tell GMT this is a netCDF file */

	if ((fp = GMT_fopen (GMT, path, "r")) == NULL)  {	/* Error in opening file */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_ncfile: Error opening file %s\n", fname);
     		return (GMT_GRDIO_READ_FAILED);
	}

	z = GMT_memory (GMT, NULL, s->n_out_columns, double *);
	for (i = 0; i < s->n_out_columns; i++) z[i] = GMT_memory (GMT, NULL, GMT->current.io.ndim, double);

	for (j = 0; j < GMT->current.io.ndim; j++) {
		if ((in = GMT->current.io.input (GMT, fp, &n_expect, &n_fields)) == NULL || n_fields != ns) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_ncfile: Error reading file %s at record %d\n", fname, j);
	     		return (GMT_GRDIO_READ_FAILED);
		}
		for (i = 0; i < s->n_out_columns; i++) z[i][j] = in[i];
	}
	strncpy (p->name, fname, 32U);
	p->n_rows = GMT->current.io.ndim;
	p->ms_rec = NULL;
	p->n_segments = 0;
	p->year = 0;
	GMT_fclose (GMT, fp);

	*data = z;
	*n_rec = p->n_rows;

	return (X2SYS_NOERROR);
}

int x2sys_read_list (struct GMT_CTRL *GMT, char *file, char ***list, unsigned int *nf)
{
	unsigned int n = 0;
	size_t n_alloc = GMT_CHUNK;
	char **p = NULL, line[GMT_BUFSIZ] = {""}, name[GMT_LEN64] = {""};
	FILE *fp = NULL;

	*list = NULL;	*nf = 0;
	if ((fp = x2sys_fopen (GMT, file, "r")) == NULL) {
  		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_list : Cannot find track list file %s in either current or X2SYS_HOME directories\n", file);
		return (GMT_GRDIO_FILE_NOT_FOUND);
	}

	p = GMT_memory (GMT, NULL, n_alloc, char *);

	while (fgets (line, GMT_BUFSIZ, fp)) {
		GMT_chop (line);	/* Remove trailing CR or LF */
		sscanf (line, "%s", name);
		p[n] = strdup (name);
		n++;
		if (n == n_alloc) {
			n_alloc <<= 1;
			p = GMT_memory (GMT, p, n_alloc, char *);
		}
	}
	fclose (fp);

	p = GMT_memory (GMT, p, n, char *);

	*list = p;
	*nf = n;

	return (X2SYS_NOERROR);
}

int x2sys_read_weights (struct GMT_CTRL *GMT, char *file, char ***list, double **weights, unsigned int *nf)
{
	unsigned int n = 0;
	size_t n_alloc = GMT_CHUNK;
	char **p = NULL, line[GMT_BUFSIZ] = {""}, name[GMT_LEN64] = {""};
	double *W = NULL, this_w;
	FILE *fp = NULL;

	*list = NULL;	*weights = NULL, *nf = 0;
	if ((fp = x2sys_fopen (GMT, file, "r")) == NULL) return (GMT_GRDIO_FILE_NOT_FOUND);	/* Quietly return if no file is found since name may be a weight */

	p = GMT_memory (GMT, NULL, n_alloc, char *);
	W = GMT_memory (GMT, NULL, n_alloc, double);

	while (fgets (line, GMT_BUFSIZ, fp)) {
		GMT_chop (line);	/* Remove trailing CR or LF */
		if (sscanf (line, "%s %lg", name, &this_w) != 2) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "x2sys_read_weights : Error parsing file %s near line %d\n", file, n);
			return (GMT_GRDIO_FILE_NOT_FOUND);

		}
		p[n] = strdup (name);
		W[n] = this_w;
		n++;
		if (n == n_alloc) {
			n_alloc <<= 1;
			p = GMT_memory (GMT, p, n_alloc, char *);
		}
	}
	fclose (fp);

	p = GMT_memory (GMT, p, n, char *);
	W = GMT_memory (GMT, W, n_alloc, double);

	*list = p;
	*weights = W;
	*nf = n;

	return (X2SYS_NOERROR);
}

void x2sys_free_list (struct GMT_CTRL *GMT, char **list, uint64_t n)
{	/* Properly free memory allocated by x2sys_read_list */
	uint64_t i;
	for (i = 0; i < n; i++) free (list[i]);
	if (list) GMT_free (GMT, list);
}

int x2sys_set_system (struct GMT_CTRL *GMT, char *TAG, struct X2SYS_INFO **S, struct X2SYS_BIX *B, struct GMT_IO *G)
{
	char tag_file[GMT_BUFSIZ] = {""}, line[GMT_BUFSIZ] = {""}, p[GMT_BUFSIZ] = {""}, sfile[GMT_BUFSIZ] = {""}, suffix[16] = {""}, unit[2][2];
	unsigned int n, k, pos = 0, geodetic = 0;
	int dist_flag = 0;
	bool geographic = false, parsed_command_R = false, n_given[2] = {false, false}, c_given = false;
	double dist, save_R_wesn[4];
	FILE *fp = NULL;
	struct X2SYS_INFO *s = NULL;

	if (!TAG) return (X2SYS_TAG_NOT_SET);

	x2sys_set_home (GMT);

	GMT_memset (B, 1, struct X2SYS_BIX);
	GMT_memset (unit, 4, char);
	B->inc[GMT_X] = B->inc[GMT_Y] = 1.0;
	B->wesn[XLO] = 0.0;	B->wesn[XHI] = 360.0;	B->wesn[YLO] = -90.0;	B->wesn[YHI] = +90.0;
	B->time_gap = B->dist_gap = dist = DBL_MAX;	/* Default is no data gap */
	B->periodic = false;

	sprintf (tag_file, "%s/%s.tag", TAG, TAG);
	if ((fp = x2sys_fopen (GMT, tag_file, "r")) == NULL) {	/* Not in current directory */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Could not find/open file %s either in current of X2SYS_HOME directories\n", tag_file);
		return (GMT_GRDIO_FILE_NOT_FOUND);
	}

	while (fgets (line, GMT_BUFSIZ, fp) && line[0] == '#');	/* Skip comment records */
	GMT_chop (line);	/* Remove trailing CR or LF */

	while ((GMT_strtok (line, " \t", &pos, p))) {	/* Process the -C -D -G -I -N -m -R -W arguments from the header */
		if (p[0] == '-') {
			switch (p[1]) {
				/* Common parameters */
				case 'R':	/* Must be smart enough to deal with any command-line -R setting also given by user */
					if (GMT->common.R.active) {	/* Have already parsed a command line setting */
						parsed_command_R = true;	GMT->common.R.active = false;	/* Set to false so 2nd parse will work */
						GMT_memcpy (save_R_wesn, GMT->common.R.wesn, 4, double);	/* Save command-line -R values */
					}
					if (GMT_parse_common_options (GMT, "R", 'R', &p[2])) {
						GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error processing %s setting in %s!\n", &p[1], tag_file);
						return (GMT_GRDIO_READ_FAILED);
					}
					GMT_memcpy (B->wesn, GMT->common.R.wesn, 4, double);
					if (parsed_command_R) GMT_memcpy (GMT->common.R.wesn, save_R_wesn, 4, double);	/* Restore command-line -R values */
					GMT->common.R.active = parsed_command_R;	/* Only true if command-line -R was parsed, not this tag file */
					break;

				case 'M':	/* GMT4 Backwards compatibility */
				case 'm':
					if (GMT_compat_check (GMT, 4))
						GMT_Report (GMT->parent, GMT_MSG_COMPAT, "Warning: Option -%c is deprecated. Segment headers are automatically identified.\n", p[1]);
					else {
						GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Bad arg in x2sys_set_system! (%s)\n", p);
						return (X2SYS_BAD_ARG);
					}
					break;
				/* Supplemental parameters */

				case 'C':	/* Distance calculation flag */
					if (p[2] == 'c') dist_flag = 0;
					if (p[2] == 'f') dist_flag = 1;
					if (p[2] == 'g') dist_flag = 2;
					if (p[2] == 'e') dist_flag = 3;
					if (dist_flag < 0 || dist_flag > 3) {
						GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error processing %s setting in %s!\n", &p[1], tag_file);
						return (X2SYS_BAD_ARG);
					}
					c_given = true;
					break;
				case 'D':
					strcpy (sfile, &p[2]);
					break;
				case 'E':
					strcpy (suffix, &p[2]);
					break;
				case 'G':	/* Geographical coordinates, set discontinuity */
					geographic = true;
					geodetic = GMT_IS_0_TO_P360_RANGE;
					if (p[2] == 'd') geodetic = GMT_IS_M180_TO_P180_RANGE;
					break;
				case 'I':
					if (GMT_getinc (GMT, &p[2], B->inc)) {
						GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error processing %s setting in %s!\n", &p[1], tag_file);
						return (GMT_GRDIO_READ_FAILED);
					}
					break;
				case 'N':	/* Distance and speed unit selection */
					switch (p[2]) {
						case 'd':	/* Distance unit selection */
							k = X2SYS_DIST_SELECTION;
							break;
						case 's':	/* Speed unit selection */
							k = X2SYS_SPEED_SELECTION;
							break;
						default:
							GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error processing %s setting in %s!\n", &p[1], tag_file);
							return (X2SYS_BAD_ARG);
							break;
					}
					if (k == X2SYS_DIST_SELECTION || k == X2SYS_SPEED_SELECTION) {
						unit[k][0] = p[3];
						if (!strchr ("cefkMn", (int)unit[k][0])) {
							GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error processing %s setting in %s!\n", &p[1], tag_file);
							return (X2SYS_BAD_ARG);
						}
						n_given[k] = true;
					}
					break;
				case 'W':
					switch (p[2]) {
						case 't':
							B->time_gap = atof (&p[3]);
							break;
						case 'd':
							B->dist_gap = atof (&p[3]);
							break;
						default:	/* Backwards compatible with old -Wtgap/dgap option */
							n = sscanf (&p[2], "%lf/%lf", &B->time_gap, &dist);
							if (n == 2) B->dist_gap = dist;
						break;
					}
					break;
				default:
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Bad arg in x2sys_set_system! (%s)\n", p);
					return (X2SYS_BAD_ARG);
					break;
			}
		}
	}
	x2sys_err_pass (GMT, x2sys_fclose (GMT, tag_file, fp), tag_file);

	x2sys_err_pass (GMT, x2sys_initialize (GMT, TAG, sfile, G, &s), sfile);	/* Initialize X2SYS and info structure */

	if (!strcmp (s->info[s->x_col].name, "lon") && !geographic) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your data have longitude but geographic (-G) not specified!\n");
		return (X2SYS_CONFLICTING_ARGS);
	}
	if (!strcmp (s->info[s->y_col].name, "lat") && !geographic) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your data have latitude but geographic (-G) not specified!\n");
		return (X2SYS_CONFLICTING_ARGS);
	}
	if (!strcmp (s->info[s->x_col].name, "x") && geographic) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your data have Cartesian x but geographic was specified!\n");
		return (X2SYS_CONFLICTING_ARGS);
	}
	if (!strcmp (s->info[s->y_col].name, "y") && geographic) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your data have Cartesian y but geographic was specified!\n");
		return (X2SYS_CONFLICTING_ARGS);
	}
	if (B->time_gap < 0.0) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error -Wt: maximum gap must be > 0!\n");
		exit (EXIT_FAILURE);
	}
	if (B->dist_gap < 0.0) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error -Wd: maximum gap must be > 0!\n");
		exit (EXIT_FAILURE);
	}
	if (geographic) {
		if (! (n_given[X2SYS_DIST_SELECTION] || n_given[X2SYS_SPEED_SELECTION])) {	/* Set defaults for geographic data */
			unit[X2SYS_DIST_SELECTION][0] = 'k';
			unit[X2SYS_SPEED_SELECTION][0] = GMT_MAP_DIST_UNIT;
			n_given[X2SYS_DIST_SELECTION] = n_given[X2SYS_SPEED_SELECTION] = true;
		}
		if (!c_given) {	/* Default is great circle distances */
			dist_flag = 2;
			c_given = true;
		}
		if (geodetic == 0 && (B->wesn[XLO] < 0 || B->wesn[XHI] < 0)) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your -R and -G settings are contradicting each other!\n");
			return (X2SYS_CONFLICTING_ARGS);
		}
		else if  (geodetic == 2 && (B->wesn[XLO] > 0 && B->wesn[XHI] > 0)) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your -R and -G settings are contradicting each other!\n");
			return (X2SYS_CONFLICTING_ARGS);
		}
		if (c_given && dist_flag == 0) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your -C and -G settings are contradicting each other!\n");
			return (X2SYS_CONFLICTING_ARGS);
		}
		if (n_given[X2SYS_DIST_SELECTION] && unit[X2SYS_DIST_SELECTION][0] == 'c') {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your -Nd and -G settings are contradicting each other!\n");
			return (X2SYS_CONFLICTING_ARGS);
		}
		if (n_given[X2SYS_SPEED_SELECTION] && unit[X2SYS_SPEED_SELECTION][0] == 'c') {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Your -Ns and -G settings are contradicting each other!\n");
			return (X2SYS_CONFLICTING_ARGS);
		}
		s->geographic = true;
		s->geodetic = geodetic;	/* Override setting */
		if (GMT_360_RANGE (B->wesn[XHI], B->wesn[XLO]))
			B->periodic = true;
	}
	if (n_given[X2SYS_DIST_SELECTION]) s->unit[X2SYS_DIST_SELECTION][0] = unit[X2SYS_DIST_SELECTION][0];
	if (n_given[X2SYS_SPEED_SELECTION]) s->unit[X2SYS_SPEED_SELECTION][0] = unit[X2SYS_SPEED_SELECTION][0];
	if (c_given) s->dist_flag = dist_flag;
	s->multi_segment = true;
	s->ms_flag = GMT->current.setting.io_seg_marker[GMT_IN];
	if (suffix[0])
		strcpy (s->suffix, suffix);
	else
		strcpy (s->suffix, sfile);

	x2sys_path_init (GMT, s);		/* Prepare directory paths to data */

	*S = s;
	return (X2SYS_NOERROR);
}

void x2sys_bix_init (struct GMT_CTRL *GMT, struct X2SYS_BIX *B, bool alloc)
{
	B->i_bin_x = 1.0 / B->inc[GMT_X];
	B->i_bin_y = 1.0 / B->inc[GMT_Y];
	B->nx_bin = irint ((B->wesn[XHI] - B->wesn[XLO]) * B->i_bin_x);
	B->ny_bin = irint ((B->wesn[YHI] - B->wesn[YLO]) * B->i_bin_y);
	B->nm_bin = B->nx_bin * B->ny_bin;
	if (alloc) B->binflag = GMT_memory (GMT, NULL, B->nm_bin, unsigned int);
}

struct X2SYS_BIX_TRACK_INFO *x2sys_bix_make_entry (struct GMT_CTRL *GMT, char *name, uint32_t id_no, uint32_t flag)
{
	struct X2SYS_BIX_TRACK_INFO *I = GMT_memory (GMT, NULL, 1, struct X2SYS_BIX_TRACK_INFO);
	I->trackname = strdup (name);
	I->track_id = id_no;
	I->flag = flag;
	I->next_info = NULL;
	return (I);
}

struct X2SYS_BIX_TRACK *x2sys_bix_make_track (struct GMT_CTRL *GMT, uint32_t id, uint32_t flag)
{
	struct X2SYS_BIX_TRACK *T = GMT_memory (GMT, NULL, 1, struct X2SYS_BIX_TRACK);
	T->track_id = id;
	T->track_flag = flag;
	T->next_track = NULL;
	return (T);
}

int x2sys_bix_read_tracks (struct GMT_CTRL *GMT, struct X2SYS_INFO *S, struct X2SYS_BIX *B, int mode, uint32_t *ID)
{
	/* Reads the binned track listing which is ASCII.  32-bit limitation on flags and IDs */
	/* mode = 0 gives linked list [for use in x2sys_put], mode = 1 gives fixed array [for use in x2sys_get] */
	uint32_t id, flag, last_id = 0;
	size_t n_alloc = GMT_CHUNK;
	char track_file[GMT_BUFSIZ] = {""}, track_path[GMT_BUFSIZ] = {""}, line[GMT_BUFSIZ] = {""}, name[GMT_BUFSIZ] = {""};
	FILE *ftrack = NULL;
	struct X2SYS_BIX_TRACK_INFO *this_info = NULL;

	sprintf (track_file, "%s/%s_tracks.d", S->TAG, S->TAG);
	x2sys_path (GMT, track_file, track_path);

	if ((ftrack = fopen (track_path, "r")) == NULL) return (GMT_GRDIO_FILE_NOT_FOUND);

	if (mode == 1)
		B->head = GMT_memory (GMT, NULL, n_alloc, struct X2SYS_BIX_TRACK_INFO);
	else
		B->head = this_info = x2sys_bix_make_entry (GMT, "-", 0, 0);	/* The head cruise is not real and has name "-"; it is our list anchor */

	if (!fgets (line, GMT_BUFSIZ, ftrack)) {	/* Skip header record */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error in header record\n");
		exit (EXIT_FAILURE);
	}
	GMT_chop (line);	/* Remove trailing CR or LF */
	if (strcmp (&line[2], S->TAG)) {	/* Mismatch between database tag and present tag */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "track data file %s lists tag as %s but active tag is %s\n",  track_path, &line[2], S->TAG);
		exit (EXIT_FAILURE);
	}
	while (fgets (line, GMT_BUFSIZ, ftrack)) {
		GMT_chop (line);	/* Remove trailing CR or LF */
		sscanf (line, "%s %d %d", name, &id, &flag);
		if (mode == 1) {	/* Add to array */
			if (id >= n_alloc) {
				size_t old_n_alloc = n_alloc;
				while (id >= n_alloc) n_alloc += GMT_CHUNK;
				B->head = GMT_memory (GMT, B->head, n_alloc, struct X2SYS_BIX_TRACK_INFO);
				GMT_memset (&(B->head[old_n_alloc]), n_alloc - old_n_alloc, struct X2SYS_BIX_TRACK_INFO);	/* Set content of new space to NULL */
			}
			B->head[id].track_id = id;
			B->head[id].flag = flag;
			B->head[id].trackname = strdup (name);
		}
		else {	/* Append to linked list */
			this_info->next_info = x2sys_bix_make_entry (GMT, name, id, flag);
			this_info = this_info->next_info;
		}
		if (id > last_id) last_id = id;
	}
	fclose (ftrack);
	last_id++;
	if (mode == 1) B->head = GMT_memory (GMT, B->head, last_id, struct X2SYS_BIX_TRACK_INFO);

	*ID = last_id;

	return (X2SYS_NOERROR);
}

int x2sys_bix_read_index (struct GMT_CTRL *GMT, struct X2SYS_INFO *S, struct X2SYS_BIX *B, bool swap)
{
	/* Reads the binned index file which is native binary and thus swab is an issue */
	char index_file[GMT_BUFSIZ] = {""}, index_path[GMT_BUFSIZ] = {""};
	FILE *fbin = NULL;
	uint32_t i, index = 0, flag, no_of_tracks, id; /* These must remain uint32_t */

	sprintf (index_file, "%s/%s_index.b", S->TAG, S->TAG);
	x2sys_path (GMT, index_file, index_path);

	if ((fbin = fopen (index_path, "rb")) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Could not open %s\n", index_path);
		return (GMT_GRDIO_OPEN_FAILED);
	}
	B->base = GMT_memory (GMT, NULL, B->nm_bin, struct X2SYS_BIX_DATABASE);

	while ((fread (&index, sizeof (uint32_t), 1U, fbin)) == 1U) {
		if (fread (&no_of_tracks, sizeof (uint32_t), 1U, fbin) != 1U) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error bin index file\n");
			return (GMT_GRDIO_READ_FAILED);
		}
		if (swap) {
			index = bswap32 (index);
			no_of_tracks = bswap32 (no_of_tracks);
		}
		B->base[index].first_track = B->base[index].last_track = x2sys_bix_make_track (GMT, 0, 0);
		for (i = 0; i < no_of_tracks; i++) {
			if (fread (&id, sizeof (uint32_t), 1U, fbin) != 1U) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error bin index file\n");
				return (GMT_GRDIO_READ_FAILED);
			}
			if (fread (&flag, sizeof (uint32_t), 1U, fbin) != 1U) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error bin index file\n");
				return (GMT_GRDIO_READ_FAILED);
			}
			if (swap) {
				id = bswap32 (id);
				flag = bswap32 (flag);
			}
			B->base[index].last_track->next_track = x2sys_bix_make_track (GMT, id, flag);
			B->base[index].last_track = B->base[index].last_track->next_track;
			B->base[index].n_tracks++;
		}
	}
	fclose (fbin);
	return (X2SYS_NOERROR);
}

int x2sys_bix_get_index (struct GMT_CTRL *GMT, double x, double y, int *i, int *j, struct X2SYS_BIX *B, uint64_t *ID)
{
	uint64_t index = 0;
	int64_t tmp;

	*j = (y == B->wesn[YHI]) ? B->ny_bin - 1 : irint (floor ((y - B->wesn[YLO]) * B->i_bin_y));
	if ((*j) < 0 || (*j) >= B->ny_bin) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "row (%d) outside range implied by -R -I! [0-%d>\n", *j, B->ny_bin);
		return (X2SYS_BIX_BAD_ROW);
	}
	*i = (x == B->wesn[XHI]) ? B->nx_bin - 1 : irint (floor ((x - B->wesn[XLO])  * B->i_bin_x));
	if (B->periodic) {
		while (*i < 0) *i += B->nx_bin;
		while (*i >= B->nx_bin) *i -= B->nx_bin;
	}
	if ((*i) < 0 || (*i) >= B->nx_bin) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "col (%d) outside range implied by -R -I! [0-%d>\n", *i, B->nx_bin);
		return (X2SYS_BIX_BAD_COL);
	}
	tmp = (*j) * B->nx_bin + (*i);
	if (tmp < 0 || (index = tmp) >= B->nm_bin) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Index (%" PRIu64 ") outside range implied by -R -I! [0-%" PRIu64 ">\n", tmp, B->nm_bin);
		return (X2SYS_BIX_BAD_INDEX);
	}

	*ID  = index;
	return (X2SYS_NOERROR);
}

/* x2sys_path_init reads the X2SYS_HOME/TAG/TAG_paths.txt file and gets all
 * the data directories (if any) for this TAG.
 */

void x2sys_path_init (struct GMT_CTRL *GMT, struct X2SYS_INFO *S)
{
	char file[GMT_BUFSIZ] = {""}, line[GMT_BUFSIZ] = {""};
	FILE *fp = NULL;

	x2sys_set_home (GMT);

	sprintf (file, "%s/%s/%s_paths.txt", X2SYS_HOME, S->TAG, S->TAG);

	n_x2sys_paths = 0;

	if ((fp = fopen (file, "r")) == NULL) {
		if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: path file %s for %s files not found\n", file, S->TAG);
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "(Will only look in current directory for such files)\n");
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "(mgd77[+] also looks in MGD77_HOME and mgg looks in GMT_SHAREDIR/mgg)\n");
		}
		return;
	}

	while (fgets (line, GMT_BUFSIZ, fp) && n_x2sys_paths < MAX_DATA_PATHS) {
		if (line[0] == '#') continue;	/* Comments */
		if (line[0] == ' ' || line[0] == '\0') continue;	/* Blank line */
		GMT_chop (line);	/* Remove trailing CR or LF */
#ifdef WIN32
		DOS_path_fix (line);
#endif
		x2sys_datadir[n_x2sys_paths] = GMT_memory (GMT, NULL, strlen (line)+1, char);
		strcpy (x2sys_datadir[n_x2sys_paths], line);
		n_x2sys_paths++;
		if (n_x2sys_paths == MAX_DATA_PATHS) GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Reached maximum directory (%d) count in %s!\n", MAX_DATA_PATHS, file);
	}
	fclose (fp);
}

/* x2sys_get_data_path takes a track name as argument and returns the full path
 * to where this data file can be found.  x2sys_path_init must be called first.
 */

int x2sys_get_data_path (struct GMT_CTRL *GMT_UNUSED(GMT), char *track_path, char *track, char *suffix)
{
	unsigned int id;
	bool add_suffix;
	char geo_path[GMT_BUFSIZ] = {""};

	/* Check if we need to append suffix */

	add_suffix = strncmp (&track[strlen(track)-strlen(suffix)], suffix, strlen(suffix));	/* Need to add suffix? */

	if (track[0] == '/' || track[1] == ':') {	/* Full path given, just return it, possibly after appending suffix */
		if (add_suffix)
			sprintf (track_path, "%s.%s", track, suffix);
		else
			strcpy (track_path, track);
		return (0);
	}

	/* First look in current directory */

	if (add_suffix)
		sprintf (geo_path, "%s.%s", track, suffix);
	else
		strcpy (geo_path, track);
	if (!access(geo_path, R_OK)) {
		strcpy(track_path, geo_path);
		return (0);
	}

	/* Then look elsewhere */

	for (id = 0; id < n_x2sys_paths; id++) {
		if (add_suffix)
			sprintf (geo_path, "%s/%s.%s", x2sys_datadir[id], track, suffix);
		else
			sprintf (geo_path, "%s/%s", x2sys_datadir[id], track);
		if (!access (geo_path, R_OK)) {
			strcpy (track_path, geo_path);
			return (0);
		}
	}
	return(1);	/* Schwinehund! */
}

const char * x2sys_strerror (struct GMT_CTRL *GMT_UNUSED(GMT), int err)
{
/* Returns the error string for a given error code "err"
   Passes "err" on to nc_strerror if the error code is not one we defined */
	switch (err) {
		case X2SYS_FCLOSE_ERR:
			return "Error from fclose";
		case X2SYS_BAD_DEF:
			return "Cannot find format definition file in either current or X2SYS_HOME directories";
		case X2SYS_BAD_COL:
			return "Unrecognized string";
		case X2SYS_TAG_NOT_SET:
			return "TAG has not been set";
		case X2SYS_BAD_ARG:
			return "Unrecognized argument";
		case X2SYS_CONFLICTING_ARGS:
			return "Conflicting arguments";
		case X2SYS_BIX_BAD_ROW:
			return "Bad row index";
		case X2SYS_BIX_BAD_COL:
			return "Bad col index";
		case X2SYS_BIX_BAD_INDEX:
			return "Bad bin index";
		default:	/* default passes through to GMT error */
			return GMT_strerror(err);
	}
}

int x2sys_err_pass (struct GMT_CTRL *GMT, int err, char *file)
{
	if (err == X2SYS_NOERROR) return (err);
	/* When error code is non-zero: print error message and pass error code on */
	if (file && file[0])
		GMT_message (GMT, "%s: %s [%s]\n", X2SYS_program, x2sys_strerror(GMT, err), file);
	else
		GMT_message (GMT, "%s: %s\n", X2SYS_program, x2sys_strerror(GMT, err));
	return (err);
}

int x2sys_err_fail (struct GMT_CTRL *GMT, int err, char *file)
{
	if (err == X2SYS_NOERROR) return X2SYS_NOERROR;
	/* When error code is non-zero: print error message and exit */
	if (file && file[0])
		GMT_message (GMT, "%s: %s [%s]\n", X2SYS_program, x2sys_strerror(GMT, err), file);
	else
		GMT_message (GMT, "%s: %s\n", X2SYS_program, x2sys_strerror(GMT, err));
	GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
}

/* Functions dealing with the reading of the COE ascii database */

uint64_t x2sys_read_coe_dbase (struct GMT_CTRL *GMT, struct X2SYS_INFO *S, char *dbase, char *ignorefile, double *wesn, char *fflag, int coe_kind, char *one_trk, struct X2SYS_COE_PAIR **xpairs, uint64_t *nx, uint64_t *nt)
{
	 /* S:		The X2SYS_INFO structure
	 * dbase:	Name of the crossover data file [NULL for stdin]
	 * ignorefile:	Name of file with track names to ignore [or NULL if none]
	 * wesn:	Rectangular box to limit COE locations [NULL or 4-array with all zeros means no limit]
	 * fflag:	The name of the chosen field (e.g., faa)
	 * coe_kind: 	1 for internal, 2 for external, 3 [or 0] for both
	 * one_trk:	NULL to get coes from all pairs; give a track name for pairs only involving that track
	 * xpairs:	The return array of pair structures; number of pairs returned by function call
	 */

	FILE *fp = NULL;
	struct X2SYS_COE_PAIR *P = NULL;
	char line[GMT_BUFSIZ] = {""}, txt[GMT_BUFSIZ] = {""}, kind[GMT_BUFSIZ] = {""}, fmt[GMT_BUFSIZ] = {""}, trk[2][GMT_LEN64], t_txt[2][GMT_LEN64], start[2][GMT_LEN64];
	char x_txt[GMT_LEN64] = {""}, y_txt[GMT_LEN64] = {""}, d_txt[2][GMT_LEN64], h_txt[2][GMT_LEN64], v_txt[2][GMT_LEN64], z_txt[2][GMT_LEN64];
	char stop[2][GMT_LEN64], info[2][3*GMT_LEN64], **trk_list = NULL, **ignore = NULL, *t = NULL;
	int i, year[2], our_item = -1, n_items, s_id;
	unsigned int id[2], n_ignore = 0, n_tracks = 0;
	bool more, skip, two_values = false, check_box, keep = true, no_time = false;
	size_t n_alloc_x, n_alloc_p, n_alloc_t;
	uint64_t k, p, n_pairs, rec_no = 0;
	double x, m, lon, dist[2], d_val;

	fp = stdin;	/* Default to stdin if dbase is NULL */
	if (dbase && (fp = fopen (dbase, "r")) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Unable to open crossover file %s\n", dbase);
		exit (EXIT_FAILURE);
	}

	n_alloc_p = n_alloc_t = GMT_CHUNK;
	P = GMT_memory (GMT, NULL, n_alloc_p, struct X2SYS_COE_PAIR);

	while (fgets (line, GMT_BUFSIZ, fp) && line[0] == '#') {	/* Process header recs */
		rec_no++;
		GMT_chop (line);	/* Get rid of [CR]LF */
		/* Looking to process these two [three] key lines:
		 * # Tag: MGD77
		   # Command: x2sys_cross ... [in later versions only]
		   # lon	lat	t_1|i_1	t_2|i_2	dist_1	dist_2	head_1	head_2	vel_1	vel_2	twt_1	twt_2	depth_1	depth_2	...
		 */
		if (!strncmp (line, "# Tag:", 6)) {	/* Found the # TAG record */
			if (strcmp (S->TAG, &line[7])) {	/* -Ttag and this TAG do not match */
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Crossover file %s has a tag (%s) that differs from specified tag (%s) - aborting\n", dbase, &line[7], S->TAG);
				exit (EXIT_FAILURE);
			}
			continue;	/* Goto next record */
		}
		if (!strncmp (line, "# Command:", 10)) {	/* Found the # Command record */
			continue;	/* Goto next record */
		}
		if (!strncmp (line, "# ", 2)) {	/* Found the possible # lon lat ... record */
			sscanf (&line[2], "%*s %*s %s %*s %*s %*s %*s %*s %*s %*s %s", kind, txt);	/* Get first column name after lon/x etc */
			if (strchr (txt, '_')) {	/* A column name with underscore; we thus assume this is the correct record */
				char ptr[GMT_BUFSIZ] = {""};
				unsigned int pos = 0, item = 0;
				no_time = !strcmp (kind, "i_1");	/* No time in this database */
				if (txt[strlen(txt)-1] == '1') two_values = true;	/* Option -2 was used */
				while (our_item == -1 && (GMT_strtok (&line[2], " \t", &pos, ptr))) {    /* Process all tokens */
					item++;
					i = (int)strlen (ptr) - 1;
					while (i >= 0 && ptr[i] != '_') i--;	/* Start at end and find last underscore */
					if (i < 0) continue;		/* First records 'lon' & 'lat' have no '_' */
					strncpy (txt, ptr, i);
					txt[i] = '\0';
					if (!strcmp (txt, fflag)) our_item = item;	/* Found the desired column */
				}
			}
		}
	}

	/* Here, line holds the next record which will be the first > ... multisegment header for a crossing pair */

	our_item -= 10;		/* Account for the 10 common items */
	if (our_item < 0) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Crossover file %s does not have the specified column %s - aborting\n", dbase, fflag);
		exit (EXIT_FAILURE);
	}

	if (ignorefile && (k = x2sys_read_list (GMT, ignorefile, &ignore, &n_ignore)) != X2SYS_NOERROR) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Ignore file %s cannot be read - aborting\n", ignorefile);
		exit (EXIT_FAILURE);
	}

	check_box = (wesn && !(wesn[XLO] == wesn[XHI] && wesn[YLO] == wesn[YHI]));	/* Specified a rectangular box */

	/* OK, our file has the required column name, lets build the format statement */

	sprintf (fmt, "%%s %%s %%s %%s %%s %%s %%s %%s %%s %%s");	/* The standard 10 items up front */
	for (i = 1; i < our_item; i++) strcat (fmt, " %*s");	/* The items to skip */
	strcat (fmt, " %s %s");	/* The item we want */

	trk_list = GMT_memory (GMT, NULL, n_alloc_t, char *);

	more = true;
	n_pairs = *nx = 0;
	while (more) {	/* Read dbase until EOF */
		GMT_chop (line);	/* Get rid of [CR]LF */
		if (line[0] == '#') {	/* Skip a comment lines */
			while (fgets (line, GMT_BUFSIZ, fp) && line[0] == '#') rec_no++;	/* Skip header recs */
			continue;	/* Return to top of while loop */
		}
		if (line[0] != '>') {	/* Trouble */
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: No segment header found [line %" PRIu64 "]\n", rec_no);
			exit (EXIT_FAILURE);
		}
		n_items = sscanf (&line[2], "%s %d %s %d %s %s", trk[0], &year[0], trk[1], &year[1], info[0], info[1]);
		for (k = 0; k < strlen (trk[0]); k++) if (trk[0][k] == '.') trk[0][k] = '\0';
		for (k = 0; k < strlen (trk[1]); k++) if (trk[1][k] == '.') trk[1][k] = '\0';
		skip = false;
		if (!(coe_kind & 1) && !strcmp (trk[0], trk[1])) skip = true;	/* Do not want internal crossovers */
		if (!(coe_kind & 2) &&  strcmp (trk[0], trk[1])) skip = true;	/* Do not want external crossovers */
		if (one_trk && (strcmp (one_trk, trk[0]) && strcmp (one_trk, trk[1]))) skip = true;	/* Looking for a specific track and these do not match */
		if (!skip && n_ignore) {	/* See if one of the tracks are in the ignore list */
			for (k = 0; !skip && k < n_ignore; k++) if (!strcmp (trk[0], ignore[k]) || !strcmp (trk[1], ignore[k])) skip = true;
		}
		if (skip) {	/* Skip this pair's data records */
			while ((t = fgets (line, GMT_BUFSIZ, fp)) && line[0] != '>') rec_no++;
			more = (t != NULL);
			continue;	/* Back to top of loop */
		}
		for (k = 0; k < 2; k++) {	/* Process each track */
			s_id = x2sys_find_track (GMT, trk[k], trk_list, n_tracks);	/* Return track id # for this leg */
			if (s_id == -1) {
				/* Leg not in the data base yet, add it */
				trk_list[n_tracks] = strdup (trk[k]);
				id[k] = n_tracks++;
				if (n_tracks == n_alloc_t) {
					n_alloc_t <<= 1;
					trk_list = GMT_memory (GMT, trk_list, n_alloc_t, char *);
				}
			}
			else
				id[k] = s_id;
		}
		/* Sanity check - make sure we dont already have this pair */
		for (p = 0, skip = false; !skip && p < n_pairs; p++) {
			if ((P[p].id[0] == id[0] && P[p].id[1] == id[1]) || (P[p].id[0] == id[1] && P[p].id[1] == id[0])) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: Pair %s and %s appear more than once - skipped [line %" PRIu64 "]\n", trk[0], trk[1], rec_no);
				skip = true;
			}
		}
		if (skip) {
			while ((t = fgets (line, GMT_BUFSIZ, fp)) && line[0] != '>') rec_no++;	/* Skip this pair's data records */
			more = (t != NULL);
			continue;	/* Back to top of loop */
		}

		/* OK, new pair */

		p = n_pairs;
		for (k = 0; k < 2; k++) {	/* Copy the values we found */
			sscanf (info[k], "%[^/]/%[^/]/%lg", start[k], stop[k], &dist[k]);
			strcpy (P[p].trk[k], trk[k]);
			P[p].id[k] = id[k];
			P[p].year[k] = year[k];
			if (n_items == 4)	/* Old format with no start/dist stuff */
				P[p].start[k] = P[p].stop[k] = dist[k] = GMT->session.d_NaN;
			else if (!strcmp (start[k], "NaN") || !strcmp (stop[k], "NaN"))	/* No time for this track */
				P[p].start[k] = P[p].stop[k] = GMT->session.d_NaN;
			else {
				if (GMT_verify_expectations (GMT, GMT_IS_ABSTIME, GMT_scanf (GMT, start[k], GMT_IS_ABSTIME, &P[p].start[k]), start[k])) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Header time specification tstart%d (%s) in wrong format [line %" PRIu64 "]\n", (k+1), start[k], rec_no);
					exit (EXIT_FAILURE);
				}
				if (GMT_verify_expectations (GMT, GMT_IS_ABSTIME, GMT_scanf (GMT, stop[k], GMT_IS_ABSTIME, &P[p].stop[k]), stop[k])) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Header time specification tstop%d (%s) in wrong format [line %" PRIu64 "]\n", (k+1), stop[k], rec_no);
					exit (EXIT_FAILURE);
				}
			}
			P[p].dist[k] = dist[k];
		}
		n_pairs++;
		if (n_pairs == n_alloc_p) {
			size_t old_n_alloc = n_alloc_p;
			n_alloc_p <<= 1;
			P = GMT_memory (GMT, P, n_alloc_p, struct X2SYS_COE_PAIR);
			GMT_memset (&(P[old_n_alloc]), n_alloc_p - old_n_alloc, struct X2SYS_COE_PAIR);	/* Set to NULL/0 */
		}
		n_alloc_x = GMT_SMALL_CHUNK;
		P[p].COE = GMT_memory (GMT, NULL, n_alloc_x, struct X2SYS_COE);
		k = 0;
		while ((t = fgets (line, GMT_BUFSIZ, fp)) && !(line[0] == '>' || line[0] == '#')) {	/* As long as we are reading data records */
			rec_no++;
			GMT_chop (line);	/* Get rid of [CR]LF */
			sscanf (line, fmt, x_txt, y_txt, t_txt[0], t_txt[1], d_txt[0], d_txt[1], h_txt[0], h_txt[1], v_txt[0], v_txt[1], z_txt[0], z_txt[1]);
			if (GMT_scanf (GMT, x_txt, GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
			P[p].COE[k].data[0][COE_X] = d_val;
			if (GMT_scanf (GMT, y_txt, GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
			P[p].COE[k].data[0][COE_Y] = d_val;

			for (i = 0; i < 2; i++) {
				if (GMT_scanf (GMT, d_txt[i], GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
				P[p].COE[k].data[i][COE_D] = d_val;

				if (GMT_scanf (GMT, h_txt[i], GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
				P[p].COE[k].data[i][COE_H] = d_val;

				if (GMT_scanf (GMT, v_txt[i], GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
				P[p].COE[k].data[i][COE_V] = d_val;

				if (GMT_scanf (GMT, z_txt[i], GMT_IS_FLOAT, &d_val) == GMT_IS_NAN) d_val = GMT->session.d_NaN;
				P[p].COE[k].data[i][COE_Z] = d_val;

				if (no_time || !strcmp (t_txt[i], "NaN"))
					P[p].COE[k].data[i][COE_T] = GMT->session.d_NaN;
				else if (GMT_verify_expectations (GMT, GMT_IS_ABSTIME, GMT_scanf (GMT, t_txt[i], GMT_IS_ABSTIME, &P[p].COE[k].data[i][COE_T]), t_txt[i])) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Time specification t%d (%s) in wrong format [line %" PRIu64 "]\n", (i+1), t_txt[i], rec_no);
					exit (EXIT_FAILURE);
				}
			}
			if (!two_values) {	/* Modify z to return the two values at the crossover point */
				x = 0.5 * P[p].COE[k].data[0][COE_Z]; m = P[p].COE[k].data[1][COE_Z];
				P[p].COE[k].data[0][COE_Z] = m + x;
				P[p].COE[k].data[1][COE_Z] = m - x;
			}
			if (check_box) {	/* Must pass coordinate check */
				keep = true;
				if (P[p].COE[k].data[0][COE_Y] < wesn[YLO] || P[p].COE[k].data[0][COE_Y] > wesn[YHI])	/* Cartesian y or latitude */
					keep = false;
				else if (S->geographic) {	/* Be cautions regarding longitude test */
					lon = P[p].COE[k].data[0][COE_X];
					while (lon > wesn[XLO]) lon -= 360.0;
					while (lon < wesn[XLO]) lon += 360.0;
					if (lon > wesn[XHI]) keep = false;
				}
				else if (P[p].COE[k].data[0][COE_X] < wesn[XLO] || P[p].COE[k].data[0][COE_X] > wesn[XHI])	/* Cartesian x */
					keep = false;
			}
			if (keep) {	/* Duplicate the coordinates at the crossover and increment k */
				P[p].COE[k].data[1][COE_X] = P[p].COE[k].data[0][COE_X];
				P[p].COE[k].data[1][COE_Y] = P[p].COE[k].data[0][COE_Y];
				k++;
			}
			if (k == n_alloc_x) {
				size_t old_n_alloc = n_alloc_x;
				n_alloc_x <<= 1;
				P[p].COE = GMT_memory (GMT, P[p].COE, n_alloc_x, struct X2SYS_COE);
				GMT_memset (&(P[p].COE[old_n_alloc]), n_alloc_x - old_n_alloc, struct X2SYS_COE);	/* Set to NULL/0 */
			}
		}
		more = (t != NULL);
		if (k == 0) {	/* No COE, probably due to wesn check */
			GMT_free (GMT, P[p].COE);
			n_pairs--;	/* To reset this value since the top of the loop will do n_pairs++ */
		}
		else {
			P[p].COE = GMT_memory (GMT, P[p].COE, k, struct X2SYS_COE);
			P[p].nx = (unsigned int)k;
			*nx += k;
		}
	}
	fclose (fp);
	if (n_pairs == 0) {	/* No pairs found, probably due to wesn check */
		GMT_free (GMT, P);
		*xpairs = NULL;
	}
	else {
		P = GMT_memory (GMT, P, n_pairs, struct X2SYS_COE_PAIR);
		*xpairs = P;
	}
	x2sys_free_list (GMT, trk_list, n_tracks);
	x2sys_free_list (GMT, ignore, n_ignore);

	*nt = n_tracks;
	return (n_pairs);
}

void x2sys_free_coe_dbase (struct GMT_CTRL *GMT, struct X2SYS_COE_PAIR *P, uint64_t np)
{	/* Free up the memory associated with P as created by x2sys_read_coe_dbase */
	uint64_t p;
	for (p = 0; p < np; p++) GMT_free (GMT, P[p].COE);
	GMT_free (GMT, P);
}

int x2sys_find_track (struct GMT_CTRL *GMT_UNUSED(GMT), char *name, char **list, unsigned int n)
{	/* Return track id # for this leg or -1 if not found */
	unsigned int i = 0;
	if (!list) return (-1);	/* Null pointer passed */
	for (i = 0; i < n; i++) if (!strcmp (name, list[i])) return (i);
	return (-1);
}

int x2sys_get_tracknames (struct GMT_CTRL *GMT, struct GMT_OPTION *options, char ***filelist, bool *cmdline)
{	/* Return list of track names given on command line or via =list mechanism.
	 * The names do not have the track extension. */
	unsigned int i, A;
	size_t n_alloc, add_chunk;
	char **file = NULL, *p = NULL;
	struct GMT_OPTION *opt = NULL, *list = NULL;

	/* Backwards checking for :list in addition to the new 9as in mgd77) =list mechanism */
	for (opt = options; !list && opt; opt = opt->next) if (opt->option == GMT_OPT_INFILE && (opt->arg[0] == ':' || opt->arg[0] == '=')) list = opt;

	if (list) {	/* Got a file with a list of filenames */
		*cmdline = false;
		if (x2sys_read_list (GMT, &list->arg[1], filelist, &A) != X2SYS_NOERROR) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Could not open list with filenames %s!\n", &list->arg[1]);
			return (-1);
		}
		file = *filelist;
	}
	else {		/* Get files from command line */
		add_chunk = n_alloc = GMT_CHUNK;
		file = GMT_memory (GMT, NULL, n_alloc, char *);

		*cmdline = true;
		for (opt = options, A = 0; opt; opt = opt->next) {
			if (opt->option != GMT_OPT_INFILE) continue;	/* Skip options */

			file[A++] = strdup (opt->arg);
			if (A == n_alloc) {
				add_chunk <<= 1;
				n_alloc += add_chunk;
				file = GMT_memory (GMT, file, n_alloc, char *);
			}
		}
		file = GMT_memory (GMT, file, A, char *);
		*filelist = file;
	}
	/* Strip off any extensions */

	for (i = 0; i < A; i++) {
		if ((p = strchr (file[i], '.'))) file[i][(size_t)(p-file[i])] = '\0';
	}

	return (A);
}

/* A very similar function (and with the same name -- but the '2') is also defined in MGD77list_func.c */
unsigned int separate_aux_columns2 (struct GMT_CTRL *GMT_UNUSED(GMT), unsigned int n_items, char **item_name, struct MGD77_AUX_INFO *aux, struct MGD77_AUXLIST *auxlist)
{	/* Used in x2sys_get_corrtable */
	unsigned int i, j, k, n_aux;
	int this_aux;
	/* Based on what item_name contains, we copy over info on the 3 aux fields (dist, azim, vel) from auxlist to aux */
	for (i = k = n_aux = 0; i < n_items; i++) {
		for (j = 0, this_aux = MGD77_NOT_SET; j < N_GENERIC_AUX && this_aux == MGD77_NOT_SET; j++) if (!strcmp (auxlist[j].name, item_name[i])) this_aux = j;
		if (this_aux != MGD77_NOT_SET) {	/* Found a request for an auxillary column  */
			aux[n_aux].type = auxlist[this_aux].type;
			aux[n_aux].text = auxlist[this_aux].text;
			aux[n_aux].pos = k;
			auxlist[this_aux].requested = true;
			n_aux++;
		}
	}
	return (n_aux);
}

void x2sys_get_corrtable (struct GMT_CTRL *GMT, struct X2SYS_INFO *S, char *ctable, uint64_t ntracks, char **trk_name, char *column, struct MGD77_AUX_INFO *aux, struct MGD77_AUXLIST *auxlist, struct MGD77_CORRTABLE ***CORR)
{	/* Load an ephemeral correction table */
	/* Pass aux as NULL if the auxillary columns do not matter (only used by x2sys_datalist) */
	unsigned int i, n_items, n_aux = 0, n_cols, missing;
	int ks;
	char path[GMT_BUFSIZ] = {""}, **item_names = NULL, **col_name = NULL, **aux_name = NULL;

	if (!ctable || !strlen(ctable)) {	/* Try default correction table */
		sprintf (path, "%s/%s/%s_corrections.txt", X2SYS_HOME, S->TAG, S->TAG);
		if (access (path, R_OK)) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "No default X2SYS Correction table (%s) for %s found!\n", path, S->TAG);
			exit (EXIT_FAILURE);
		}
		ctable = path;
	}
	if (column) {	/* Must build list of the 7 standard COE database column names */
		n_cols = 7;
		col_name = GMT_memory (GMT, NULL, n_cols, char *);
		col_name[COE_X] = (S->geographic) ? strdup ("lon") : strdup ("x");
		col_name[COE_Y] = (S->geographic) ? strdup ("lat") : strdup ("y");
		col_name[COE_T] = strdup ("time");
		col_name[COE_Z] = strdup (column);
		col_name[COE_D] = strdup ("dist");
		col_name[COE_H] = strdup ("azim");
		col_name[COE_V] = strdup ("vel");
	}
	else {	/* Use what is available in the data files */
		n_cols = S->n_out_columns;
		col_name = GMT_memory (GMT, NULL, n_cols, char *);
		for (i = 0; i < n_cols; i++) col_name[i] = strdup (S->info[S->out_order[i]].name);
	}
	n_items = MGD77_Scan_Corrtable (GMT, ctable, trk_name, (unsigned int)ntracks, n_cols, col_name, &item_names, 0);
	if (aux && (n_aux = separate_aux_columns2 (GMT, n_items, item_names, aux, auxlist))) {	/* Determine which auxillary columns are requested (if any) */
		aux_name = GMT_memory (GMT, NULL, n_aux, char *);
		for (i = 0; i < n_aux; i++) aux_name[i] = strdup (auxlist[aux[i].type].name);
	}
	for (i = missing = 0; i < n_items; i++) {
		if (MGD77_Match_List (GMT, item_names[i], n_cols, col_name) == MGD77_NOT_SET) {	/* Requested column not among data cols */
			if ((ks = MGD77_Match_List (GMT, item_names[i], n_aux, aux_name)) == MGD77_NOT_SET) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "X2SYS Correction table (%s) requires a column (%s) not present in COE database or auxillary columns\n", ctable, item_names[i]);
				missing++;
			}
			else
				auxlist[aux[ks].type].requested = true;
		}
	}
	MGD77_Free_Table (GMT, n_items, item_names);
	x2sys_free_list (GMT, aux_name, n_aux);
	if (!missing) MGD77_Parse_Corrtable (GMT, ctable, trk_name, (unsigned int)ntracks, n_cols, col_name, 0, CORR);
	x2sys_free_list (GMT, col_name, n_cols);
	if (missing) exit (EXIT_FAILURE);
}
