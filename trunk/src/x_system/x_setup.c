/*	$Id: x_setup.c,v 1.1.1.1 2000-12-28 01:23:45 gmt Exp $
 *
 * XSETUP will read the gmtindex files and create a list of
 * pairs of legs that cross the same bin. As an option, the
 * user may specify a file with a list of legs. Then, only
 * those legs and whatever legs they cross are listed. This
 * is the usual way to do things when a new batch of legs
 * have been added to the databank. !! These new legs must
 * first be run through binlegs so that xsetup can find their
 * binindeces.
 *
 * Author:	Paul Wessel
 * Date:	15-NOV-1987
 * Revised:	18-FEB-1989
 * 		15-FEB-1990	PW: Fixed bug when -L is used
 * 		27-MAR-1992	PW: Updated to new indexfile format
 * 		06-MAR-2000	PW: POSIX
 *				gmt_legs.d and gmt_index.b are now
 *				in $(GMTHOME)/share/mgg
 *		17-JUL-2000	Replace MAX by MAXLEGS and use gmt.h
 *
 */
#include "gmt.h"
#include "x_system.h"

#define MAXLEGS 64000	/* Max 32000 inside and 32000 outside legs */
#define MAXNEW 500	/* Max # new legs to check [-L option] */
#define NTOT 10000	/* Max # legs in database */

struct INFO {
	char legname[16], use;
	int leg_id, seq_no;
	unsigned char *leglist;
};

struct LEGNAMES {
	int leg_no;
	char gmt;
} leg_info[NTOT];

struct INFO *make_info (char *name, int id_no);
int findleg (char *name);

int nlegs_used = 0;
char leg_used[MAXNEW][10];
unsigned int legpointer[NTOT];
struct INFO *ptr[MAXLEGS];

main (int argc, char *argv[])
{
	int i, j, byte_1, byte_2, bit_1, bit_2, leg_1, leg_2, ok;
	int lat, lon, west = 0, east = 360, south = -90, north = 89;
	long n_alloc, record;
	int bin, no;
	int nlegs, n_tot_legs;
	double w, e, s, n;
	char lname[8], line[BUFSIZ], *LIBDIR;
	unsigned char turn_on[8];
	FILE *fleg, *fbin, *fpl = NULL;
	BOOLEAN error = FALSE, skip;
	
	for (i = 1; i < argc; i++) {
  		if (argv[i][0] == '-') {
  			switch(argv[i][1]) {
  				case 'L':
  					fpl = fopen(&argv[i][2], "r");
  					break;
  				case 'R':	/* Region */
  					sscanf (&argv[i][2], "%lf/%lf/%lf/%lf", &w, &e, &s, &n);
  					west = (int) floor (w);	east = (int) ceil (e);
  					south = (int) floor (s);	north = (int) ceil (n);
  					if (west < 0) {
  						west += 360;
  						east += 360;
  					}
  					break;
  				default:
  					error = TRUE;
  					break;
  			}
  		}
  		else
  			error = TRUE;
  	}
  	if (error) {
  		fprintf(stderr,"xsetup - Prepare crossover job\n");
  		fprintf(stderr,"usage: xsetup [-L<legfile> -R<west>/<east>/<south>/<north>]\n");
  		fprintf(stderr, "	legfile is an optional list of legs to check\n");
  		fprintf(stderr, "	-R defines the region of interest. [Default is world]\n");
  		exit (EXIT_FAILURE);
  	}
  	
	if ((LIBDIR = getenv ("GMTHOME")) == (char *)NULL) {
		fprintf (stderr, "x_setup: Environment variable GMTHOME not set!\n");
		exit (EXIT_FAILURE);
	}

   	sprintf (line, "%s%cshare%cmgg%cgmt_legs.d\0", LIBDIR, DIR_DELIM, DIR_DELIM, DIR_DELIM);
 	if ((fleg = fopen (line, "r")) == NULL) {
		fprintf(stderr,"Could not open %s\n", line);
		exit (EXIT_FAILURE);
	}
   	sprintf (line, "%s%cshare%cmgg%cgmt_index.b\0", LIBDIR, DIR_DELIM, DIR_DELIM, DIR_DELIM);
	if ((fbin = fopen (line, "rb")) == NULL) {
		fprintf(stderr,"Could not open %sb\n", line);
		exit (EXIT_FAILURE);
	}
	
	if (fpl != NULL) {
		while (fgets (line, BUFSIZ, fpl)) {
			sscanf(line, "%s", leg_used[nlegs_used]);
			nlegs_used++;
		}
		if (nlegs_used > MAXNEW) {
			fprintf(stderr, "Too many new legs, recompile!\n");
			exit (EXIT_FAILURE);
		}
		fclose(fpl);
	}
	
	/* Read info about each leg */
	
	i = 0;
	while (fgets (line, BUFSIZ,fleg)) {
		sscanf(line,"%s %d",lname, &no);
		ptr[no] = make_info (lname, no);
		legpointer[i] = no;
		ptr[no]->seq_no = i++;
		if (nlegs_used > 0)	/* use only these legs */
			ptr[no]->use = (findleg (ptr[no]->legname)) ? TRUE : FALSE;
		else	/* Use all legs */
			ptr[no]->use = TRUE;
	}
	fclose(fleg);
	n_tot_legs = i;
	n_alloc = (n_tot_legs-1) / 8 + 1;
	for (i = 0; i < MAXLEGS; i++) {
		if (ptr[i]) {
			if ((ptr[i]->leglist = (unsigned char *) malloc (n_alloc)) == NULL) {
				fprintf (stderr, "xsetup: Out of memory!\n");
				exit (EXIT_FAILURE);
			}
			for (j = 0; j < n_alloc; j++)
				ptr[i]->leglist[j] = 0;
		}
	}
	
	turn_on[0] = 1;
	for (i = 1; i < 8; i++)
		turn_on[i] = turn_on[i-1]*2;


	/* Start reading the index-file and write out every pair of legs that
	 * occupy the same bin
	 */
	
	while (fread ((void *)&bin, 4, 1,fbin) != 0) {
		skip = FALSE;
		lat = (bin / 360) - 90;
		if (lat < south || lat >= north) skip = TRUE;
		if (!skip) {
			lon = bin % 360;
			while (lon < west) lon += 360;
			if (lon >= east) skip = TRUE;
		}
		fread ((void *)&nlegs, 4, 1, fbin);
		if (skip) {
			fseek (fbin, (long int)(nlegs*4), SEEK_CUR);
			continue;
		}
		for (i = 0; i < nlegs; i++) {
			fread ((void *)&record, 4, 1, fbin);
			leg_info[i].gmt = record & 15;
			leg_info[i].leg_no = (record >> 4);
		}
		for (i = 0; i < nlegs; i++) {
			leg_1 = ptr[leg_info[i].leg_no]->seq_no;
			byte_1 = leg_1 / 8;
			bit_1 = leg_1 % 8;
			for (j = i; j < nlegs; j++) {
				leg_2 = ptr[leg_info[j].leg_no]->seq_no;
				byte_2 = leg_2 / 8;
				bit_2 = leg_2 % 8;
				ptr[leg_info[i].leg_no]->leglist[byte_2] |= turn_on[bit_2];
				ptr[leg_info[j].leg_no]->leglist[byte_1] |= turn_on[bit_1];
			}
		}
	}
	fclose(fbin);
	
	for (i = 0; i < MAXLEGS; i++) {
		if (ptr[i] && ptr[i]->use) {
			for (j = 0; j < n_tot_legs; j++) {
				byte_1 = j / 8;
				bit_1 = j % 8;
				ok = ptr[i]->leglist[byte_1] & turn_on[bit_1];
				if (ok && strcmp(ptr[i]->legname, ptr[legpointer[j]]->legname) <= 0)
					printf("%s %s\n", ptr[i]->legname,ptr[legpointer[j]]->legname);
			}
		}
	}
					
}

struct INFO *make_info (char *name, int id_no)
{
	struct INFO *new;
	if ((new = (struct INFO *) malloc(sizeof(struct INFO))) == NULL) {
		fprintf(stderr,"malloc returned NULL for make_info\n");
		exit (EXIT_FAILURE);
	}
	strcpy(new->legname,name);
	new->leg_id = id_no;
	new->use = 0;
	return (new);
}


int findleg (char *name)
{
	int left, right, mid, cmp;
	
	left = 0;
	right = nlegs_used-1;
	while (left <= right) {
		mid = (left + right)/2;
		cmp = strcmp(name, leg_used[mid]);
		if (cmp < 0)
			right = mid-1;
		else if (cmp > 0)
			left = mid+1;
		else
			return (1);
	}
	return (0);
}
