/*
 *	$Id: polygon_final_info.c,v 1.4 2007-08-27 19:02:26 guru Exp $
 */
/* polygon_final_info wvs_polygon_file.b
 *
 */

#include "wvs.h"

struct GMT3_POLY h;

int main (int argc, char **argv)
{
	FILE	*fp;
	int id, src = -1, np = 0;

	if (argc < 2 || argc > 3) {
		fprintf(stderr,"usage:  polygon_final_info wvs_polygons.b [src]\n");
		exit(-1);
	}

	if (argc == 3) src = atoi (argv[2]);
	
	fp = fopen(argv[1], "r");
	
	id = 0;
	printf ("#ID\tN\tlevel\tsource\twest\t\teast\t\tsouth\t\tnorth\t\tarea\n");
	while (pol_readheader (&h, fp) == 1) {
		if (fseek (fp, (long) (h.n * sizeof (struct LONGPAIR)), SEEK_CUR)) {
			fprintf (stderr, "polygon_final_info: Failed seeking ahead\n");
			exit (-1);
		}
		
		if (src == -1 || h.source == src)
			printf ("%d\t%d\t%d\t%d\t%10.5f\t%10.5f\t%9.5f\t%9.5f\t%g\n", h.id, h.n, h.level, h.source, h.west, h.east, h.south, h.north, h.area);
		id++;
		np += h.n;
	}

	fclose(fp);

	fprintf (stderr, "polygon_final_info: Found %d polygons with a total of %d points\n", id, np);

	exit(0);
}
