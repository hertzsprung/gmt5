/*
 *	$Id: segment_report.c,v 1.1 2004-09-05 04:00:51 pwessel Exp $
 */
/* 
 *	segment_report makes a multisegment ascii-file of entire dbase
 */

#include "wvs.h"

#define M360		360000000

main (argc, argv)
int	argc;
char **argv;
{
	FILE	*fp_in;
	int	k, i, level[100];
	struct	LONGPAIR p;
	struct RAWSEG_HEADER h;
        
	if (argc != 2) {
		fprintf(stderr,"usage:  segment_report polygons.b\n");
		exit(-1);
	}
	
	memset ((char *)level, 0, 100*sizeof(int));
	
	fp_in = fopen(argv[1], "r");
		
	while (fread((char *)&h, sizeof (struct RAWSEG_HEADER), 1, fp_in) == 1) {
		level[h.rank]++;
		fseek (fp_in, h.n * sizeof(struct LONGPAIR), 1);
	}
		
	fclose(fp_in);
	
	for (i = 0; i < 100; i++) {
		if (level[i]) fprintf (stderr, "Level %d: %d segments\n", i, level[i]);
	}

	exit (0);
}
