/*	$Id$
*
* PROGRAM:   gshhstograss.c
* AUTHOR:    Simon Cox (simon@ned.dem.csiro.au),
*            Paul Wessel (wessel@soest.hawaii.edu),
*			 Markus Metz (markus_metz@gmx.de)
* DATE:	April. 27, 1996
* PURPOSE:     To extract ASCII data from binary shoreline data
*	       as described in the 1996 Wessel & Smith JGR Data Analysis Note
*	       and write files in dig_ascii format for import as GRASS vector maps
* VERSION:	
*		1.2 18-MAY-1999: Explicit binary open for DOS
* 		1.4 05-SEP-2000: Swab done automatically
*		1.5 11-SEP-2004: Updated to work with GSHHS database v1.3
*		1.6 02-MAY-2006: Updated to work with GSHHS database v1.4
*			05-SEP-2007: Removed reliance on getop and made changes
*				    	so it will compile under Windows
* 		1.7 08-APR-2008: 
*			level no longer only 1, see bug fix for gshhs 1.7
*			lat lon swapped for output, was wrong
*			added two category entries in two layers for each line
*			layer 1: cat = level, allows creation of a short table
*					containing level numbers and labels
*			layer 2: cat = unique polygon ID, with level nr, 
*                   in case unique IDs for each line are wanted
*			improved acknowledging of user-defined extends
*		1.8 31-MAR-2007: Updated to deal with latest GSHHS database (1.5)
*		1.9 27-AUG-2007: Handle line data as well as polygon data
*		1.10 15-FEB-2008: Updated to deal with latest GSHHS database (1.6)
*		1.11 15-JUN-2009: Now contains information on container polygon,
*				the polygons ancestor in the full resolution, and
*				a flag to tell if a lake is a riverlake.
*				Updated to deal with latest GSHHS database (2.0)
*		1.12 24-MAY-2010: Deal with 2.1 format.
*		1.13 1-JUL-2011: Now contains improved area information (2.2.0),
*				 and revised greenwhich flags (now 2-bit; see gshhs.h).
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU Lesser General Public License as published by
*	the Free Software Foundation; version 3 or any later version.
*
*  This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU Lesser General Public License for more details.
*/

#include "gshhs.h"
#include <string.h>

#ifdef HAVE_SYS_TYPES_H_
#	include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H_
#	include <unistd.h>
#endif

#ifdef HAVE_IO_H_
#	include <io.h>
#endif

#include <time.h>

void help_msg(char *progname);
void usage_msg (char *progname);
char *putusername();

int main (int argc, char **argv)
{
	int i = 1;
	double w, e, s, n, lon, lat;
	double minx = -360., maxx = 360., miny = -90., maxy = 90.;
	char *progname = NULL, *dataname = NULL, ascii_name[40], att1_name[40], att2_name[40];
	static char *slevel[] = { "unknown" , "land" , "lake" , "island in lake" , "pond in island in lake"};
	int shore_levels = 5;
	FILE	*fp = NULL, *ascii_fp = NULL, *att1_fp = NULL, *att2_fp = NULL;
	size_t n_read;
	int max = 270000000, flip, level, version, greenwich, shorelines;
	struct POINT p;
	struct GSHHS h;
	unsigned k, max_id = 0;
	time_t tloc;

	progname = argv[0];

	if (argc == 0) {
		usage_msg(progname);
		exit(EXIT_FAILURE);
	}

	while (i < argc) {
		if (argv[i][0] != '-') {
			fprintf (stderr, "%s:  Unrecognized argument %s.\n", progname, argv[i]);
			exit (EXIT_FAILURE);
		}
		switch (argv[i][1]) {
			case 'i':
				dataname = argv[++i];
				break;
			case 'x':
				minx = atof (argv[++i]);
				break;
			case 'X':
				maxx = atof (argv[++i]);
				break;
			case 'y':
				miny = atof (argv[++i]);
				break;
			case 'Y':
				maxy = atof(argv[++i]);
				break;
			case 'h':
				help_msg(progname);
				exit (EXIT_FAILURE);
			default:
				fprintf (stderr, "%s: Bad option %c.\n", progname, argv[i][1]);
				usage_msg(progname);
				exit (EXIT_FAILURE);
		}
		i++;
	}

	if (argc < 3 || !dataname) {
		usage_msg (progname);
		exit(EXIT_FAILURE);
	}
		
	if ((fp = fopen (dataname, "rb")) == NULL ) {
		fprintf (stderr, "%s: Could not find file %s.\n", progname, dataname);
		exit (EXIT_FAILURE);
	}
	if( minx > maxx ){
		fprintf (stderr, "%s: minx %f > maxx %f.\n", progname, minx, maxx);
		exit (EXIT_FAILURE);
	}
	if( miny > maxy ){
		fprintf (stderr, "%s: miny %f > maxy %f.\n", progname, miny, maxy);
		exit (EXIT_FAILURE);
	}

	/* now change the final . in the datafilename to a null i.e. a string terminator */
	*strrchr(dataname,056)= 0;

	shorelines = strstr(dataname,"gshhs") ? 1 : 0;

	strcpy(ascii_name,"grass_");
	strcat(ascii_name,dataname);
	strcat(ascii_name,".ascii");
	if ((ascii_fp = fopen (ascii_name, "w")) == NULL ) {
		fprintf (stderr, "%s: Could not open file %s for writing.\n", progname, ascii_name);
		exit(EXIT_FAILURE);
	}

	if (shorelines) {
		strcpy(att1_name,"grass_");
		strcat(att1_name,dataname);
		strcat(att1_name,"_layer_1.sh");
		if ((att1_fp = fopen (att1_name, "w")) == NULL ) {
			fprintf (stderr, "%s: Could not open file %s for writing.\n", progname, att1_name);
			exit(EXIT_FAILURE);
		}
	}

	strcpy(att2_name,"grass_");
	strcat(att2_name,dataname);
	strcat(att2_name,"_layer_2.sh");
	if ((att2_fp = fopen (att2_name, "w")) == NULL ) {
		fprintf (stderr, "%s: Could not open file %s for writing.\n", progname, att2_name);
		exit(EXIT_FAILURE);
	}

	/* write header for GRASS ASCII vector */
	fprintf(ascii_fp,"ORGANIZATION: \n");
	time(&tloc);
	fprintf(ascii_fp,"DIGIT DATE:   %s",ctime(&tloc));
	fprintf(ascii_fp,"DIGIT NAME:   %s\n",putusername());
	fprintf(ascii_fp,"MAP NAME:     Global Shorelines\n");
	fprintf(ascii_fp,"MAP DATE:     2004\n");
	fprintf(ascii_fp,"MAP SCALE:    1\n");
	fprintf(ascii_fp,"OTHER INFO:   \n");
	fprintf(ascii_fp,"ZONE:	       0\n");
	fprintf(ascii_fp,"WEST EDGE:    %f\n",minx);
	fprintf(ascii_fp,"EAST EDGE:    %f\n",maxx);
	fprintf(ascii_fp,"SOUTH EDGE:   %f\n",miny);
	fprintf(ascii_fp,"NORTH EDGE:   %f\n",maxy);
	fprintf(ascii_fp,"MAP THRESH:   0.0001\n");
	fprintf(ascii_fp,"VERTI:\n");


	/* prepare scripts to import categories */
	fprintf(att2_fp,"#!/bin/sh\n\n");
	fprintf(att2_fp,"# %6d categories, starting at 0\n\n",999999);
	fprintf(att2_fp,"GRASS_VECTOR=\"%s\"\n\n",dataname);

	if (shorelines) {
		fprintf(att1_fp,"#!/bin/sh\n\n");
		fprintf(att1_fp,"GRASS_VECTOR=\"%s\"\n\n",dataname);
		fprintf(att1_fp,"v.db.addtable map=$GRASS_VECTOR table=${GRASS_VECTOR}_layer_1 layer=1 \'columns=cat integer,level varchar(40)\'\n");

		fprintf(att2_fp,"v.db.addtable map=$GRASS_VECTOR table=${GRASS_VECTOR}_layer_2 layer=2 \'columns=cat integer,level_nr integer,level varchar(40)\'\n");
	}
	else {
		fprintf(att2_fp,"v.db.addtable map=$GRASS_VECTOR table=${GRASS_VECTOR}_layer_2 layer=2 \'columns=cat integer,level_nr integer\'\n");
	}

	fprintf(att2_fp,"echo \"\"\n");
	fprintf(att2_fp,"echo \"Inserting level numbers, might take some time...\"\n");

	/* read lines from binary gshhs database */
	n_read = fread (&h, sizeof (struct GSHHS), (size_t)1, fp);
	version = (h.flag >> 8) & 255;
	flip = (version != GSHHS_DATA_RELEASE);	/* Take as sign that byte-swabbing is needed */

	while (n_read == 1) {

		if (flip)
			bswap_GSHHS_struct (&h);
		level = h.flag & 255;
		version = (h.flag >> 8) & 255;
		greenwich = (h.flag >> 16) & 3;
		w = h.west * GSHHS_SCL;
		e = h.east * GSHHS_SCL;
		s = h.south * GSHHS_SCL;
		n = h.north * GSHHS_SCL;

		if( ( w <= maxx && e >= minx ) && ( s <= maxy && n >= miny ) ){
			fprintf(ascii_fp,"L %d 2\n",h.n);
			if( h.id > max_id ) max_id= h.id;

			fprintf(att2_fp,"v.db.update map=$GRASS_VECTOR layer=2 column=level_nr value=\'%d\' where=cat=%d\n",level,h.id);

			for (k = 0; k < h.n; k++) {

				if (fread (&p, sizeof (struct POINT), (size_t)1, fp) != 1) {
					fprintf (stderr, "%s:  Error reading file %s.b.\n", progname, dataname);
					exit(EXIT_FAILURE);
				}
				if (flip)
					bswap_POINT_struct (&p);
				lon = p.x * GSHHS_SCL;
				if ((greenwich & 1) && p.x > max) lon -= 360.0;
				lat = p.y * GSHHS_SCL;
				fprintf(ascii_fp," %f %f\n",lon,lat);
			}
			/* add categories in two layers
			  layer 1: cat = level
			  layer 2: cat = h.id
			*/
			fprintf(ascii_fp," 1 %d\n",level);
			fprintf(ascii_fp," 2 %d\n",h.id);
		}
		else {
			fprintf(stderr,"line %d skipped\n", h.id);
			fseek (fp, (off_t)(h.n * sizeof(struct POINT)), SEEK_CUR);
		}
		max = 180000000;	/* Only Eurasia needs 270 */

		n_read = fread (&h, sizeof (struct GSHHS), (size_t)1, fp);
	}
	
	/* don't print level names for borders and rivers */
	if (shorelines) {
		fprintf(att2_fp,"echo \"Inserting level labels, might take some time...\"\n");
		for ( level = 0; level < shore_levels; level++ ) {
			fprintf(att1_fp,"v.db.update map=$GRASS_VECTOR layer=1 column=level value=\'%s\' where=cat=%d\n",slevel[level],level);
			fprintf(att2_fp,"v.db.update map=$GRASS_VECTOR layer=2 column=level value=\'%s\' where=level_nr=%d\n",slevel[level],level);
		}
		fprintf(att2_fp,"echo \"Done.\"\n");

		fclose(att1_fp);
	}
	
	/* now fix up the number of categories */
	fseek (att2_fp, (off_t)0, 0);
	fprintf(att2_fp,"#!/bin/sh\n\n");
	fprintf(att2_fp,"# %6u categories, starting at 0\n\n",max_id + 1);

	fclose(fp);
	fclose(ascii_fp);
	fclose(att2_fp);

	exit (EXIT_SUCCESS);
}

void help_msg (char *progname) {

	fprintf (stderr,"gshhs to GRASS ASCII export tool\n\n");
	fprintf (stderr, "  %s reads *.b files of the GSHHS database and\n  writes GRASS compatible ascii vector format files.\n\n", progname);
	fprintf (stderr, "  The resulting GRASS ASCII vector file is called\n  grass_[gshhs|wdb_borders|wdb_rivers]_[f|h|i|l|c].ascii\n\n");
	fprintf (stderr, "  Scripts to import attributes to GRASS are saved as\n  grass_[gshhs|wdb_borders|wdb_rivers]_[f|h|i|l|c]_layer_[1|2].sh\n\n");
	fprintf (stderr, "  Layer 1: cat = level\n  Attribute table has level nr and level label, not produced for\n  borders and rivers.\n");
	fprintf (stderr, "  Layer 2: cat = polygon ID as in GSHHS database\n  Attribute table has polygon id, level nr and, for shorelines, level label.\n\n");
	fprintf (stderr, "  Import the *.ascii file into a GRASS database using v.in.ascii.\n");
	fprintf (stderr, "  If the GRASS vector has a name different from\n  [gshhs|wdb_borders|wdb_rivers]_[f|h|i|l|c].b,\n  change the variable GRASS_VECTOR at the beginning of the scripts.\n");
	fprintf (stderr, "  Set permission to execute the scripts, run script for desired layer.\n\n");
	
}

void usage_msg (char *progname) {

	fprintf (stderr,"gshhs to GRASS ASCII export tool\n");
	fprintf (stderr, "usage:  %s [-h] -i <input>.b [-x minx] [-X maxx] [-y miny] [-Y maxy]\n\n", progname);
	fprintf (stderr, "Arguments to %s:\n", progname);
	fprintf (stderr, " -h            print description and help\n");
	fprintf (stderr, " -i <input>.b  input file from GSHHS database\n");
	fprintf (stderr, " -x minx       western limit in decimal degrees\n");
	fprintf (stderr, " -X maxx       eastern limit in decimal degrees\n");
	fprintf (stderr, " -y miny       southern limit in decimal degrees\n");
	fprintf (stderr, " -Y maxy       northern limit in decimal degrees\n");
	fprintf (stderr, "\n");
	
}

char *putusername ()
{
	static char *unknown = "unknown";
#ifdef HAVE_GETPWUID
#include <pwd.h>
	struct passwd *pw = NULL;
	pw = getpwuid (getuid ());
	if (pw) return (pw->pw_name);
#endif
	return (unknown);
}
