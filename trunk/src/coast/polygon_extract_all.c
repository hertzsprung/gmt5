/*
 *	$Id: polygon_extract_all.c,v 1.1 2004-09-05 04:00:51 pwessel Exp $
 */
/* 
 *
 */

#include "wvs.h"

struct CHECK {
	int pos;
	struct GMT3_POLY h;
} poly[N_POLY];

main (argc, argv)
int	argc;
char **argv;
{
	FILE	*fp_in, *fp;
	int	i, j, id, n_id, k, pos, subset = 0;
	double w, e, s , n, x, y;
	struct	LONGPAIR p;
	char file[80];
        
	if (argc == 1) {
		fprintf(stderr,"usage:  polygon_extract final_polygons.b w e s n\n");
		exit(-1);
	}
	if (argc == 6) {
		subset = 1;
		w = atof (argv[2]);
		e = atof (argv[3]);
		s = atof (argv[4]);
		n = atof (argv[5]);
	}

	fp_in = fopen(argv[1], "r");
		
	n_id = pos = 0;
	while (pol_readheader (&poly[n_id].h, fp_in) == 1) {
		pos += sizeof (struct GMT3_POLY);
		poly[n_id].pos = pos;
		fseek (fp_in, poly[n_id].h.n * sizeof(struct LONGPAIR), 1);
		pos += poly[n_id].h.n * sizeof(struct LONGPAIR);
		poly[n_id].h.id = n_id;
		n_id++;
	}
	
	/* Start extraction */
	
	for (i = 0; i < n_id; i++) {
				
		fprintf (stderr, "Extracting Polygon # %d\n", poly[i].h.id);	
				
		
		sprintf (file, "polygon.%d\0", poly[i].h.id);
		fp = fopen (file, "w");
		
		fseek (fp_in, poly[i].pos, 0);
		
		for (k = 0; k < poly[i].h.n; k++) {
			if (pol_fread (&p, 1, fp_in) != 1) {
				fprintf(stderr,"polygon_extract:  ERROR  reading file.\n");
				exit(-1);
			}
			if (poly[i].h.greenwich && p.x > poly[i].h.datelon) p.x -= M360;
			
			/* fprintf (fp, "%d\t%d\n", p.x, p.y); */
			x = 1.0e-6*p.x;
			y = 1.0e-6*p.y;
			if (subset && (x < w || x > e || y < s || y > n)) continue;
			fprintf (fp, "%.10lg\t%.10lg\n", x, y);
		}
		fclose (fp);
		
	}
		
	fclose(fp_in);

	exit (0);
}
