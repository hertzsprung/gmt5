/*
 *	$Id: polygon_dump.c,v 1.1 2004-09-05 04:00:51 pwessel Exp $
 */
/* 
 *	polygon_dump makes a multisegment ascii-file of entire dbase
 */

#include "wvs.h"

main (argc, argv)
int	argc;
char **argv;
{
	FILE	*fp_in, *fp = stdout;
	int	k, level = 0, multi = 0;
	struct	LONGPAIR p;
	struct GMT3_POLY h;
	char file[80];
        
	if (argc == 1) {
		fprintf(stderr,"usage:  polygon_dump polygons.b level [-M]\n");
		exit(-1);
	}
	level = (argv[2]) ? atoi (argv[2]) : 0;
	multi = (argc == 4);
	
	fp_in = fopen(argv[1], "r");
		
	while (pol_readheader (&h, fp_in) == 1) {
		if (level > 0 && h.level != level) {
			fseek (fp_in, h.n * sizeof(struct LONGPAIR), 1);
			continue;
		}
		
		if (!multi) {
			sprintf (file, "polygon.%d\0", h.id);
			fp = fopen (file, "w");
		}
		else
			printf ("> Polygon %d N = %d\n", h.id, h.n);
			
		for (k = 0; k < h.n; k++) {
			if (pol_fread (&p, 1, fp_in) != 1) {
				fprintf(stderr,"polygon_dump:  ERROR  reading file.\n");
				exit(-1);
			}
			if (h.greenwich && p.x > h.datelon) p.x -= M360;
			
			fprintf (fp, "%lg\t%lg\n", 1.0e-6 * p.x, 1.0e-6 * p.y);
		}
		if (!multi) fclose (fp);
	}
		
	fclose(fp_in);

	exit (0);
}
