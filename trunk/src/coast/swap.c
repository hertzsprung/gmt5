/*
 *	$Id: swap.c,v 1.1 2008-03-05 01:43:03 guru Exp $
 * Based on polygon_findlevel but limited to just compute polygon areas.
 */
#include "wvs.h"
int swab_polheader (struct GMT3_POLY *h);
int swab_polpoints (struct LONGPAIR *p, int n);

int main (int argc, char **argv) {
	int k;
	FILE *fp;
	struct LONGPAIR p;
	struct GMT3_POLY h;
	
	if (argc == 1) {
		fprintf (stderr, "usage: swap final_dbase.b > unswapped.b\n");
		exit (-1);
	}

	fp = fopen (argv[1], "r");
	
	while (pol_readheader (&h, fp) == 1) {
		swab_polheader (&h);
		pol_writeheader (&h, stdout);
		for (k = 0; k < h.n; k++) {
			if (pol_fread (&p, 1, fp) != 1) {
				fprintf(stderr,"polygon_findarea:  ERROR  reading file.\n");
				exit(-1);
			}
			swab_polpoints (&p, 1);
			if (pol_fwrite (&p, 1, stdout) != 1) {
				fprintf(stderr,"polygon_findarea:  ERROR  writing file.\n");
				exit(-1);
			}
		}
	}
	
	fclose (fp);
	
	exit (0);
}	
