/*
 *	$Id: man_fix2.c,v 1.1 2004-09-05 04:00:51 pwessel Exp $
 */
#include "wvs.h"

#define N_MAX_LEN	1450000		/* 1435084 to be exact */
#define N_POLY		189000		/* 188281 to be exact */
#define M360		360000000
#define MILL		1000000

struct CHECK {
	int pos;
	struct GMT3_POLY h;
} poly[N_POLY];

struct BURP {
	int a, b, nx, f;
} x[1040];

main (argc, argv)
int argc;
char **argv; {
	int i = 0, ok, j, k, nk = 0, n_id, id, n_bad, np, nh, test, ncut = 0, sort_on_np();
	int go[2], bad, n_skip1 = 0, n_skip2 = 0;
	int w, e, s, n;
	FILE *fp, *fp_bad, *fp_in, *fp_fix, *fpj;
	char line[80], cmd[512], file[512], ss[10];
	struct	LONGPAIR p;
	
	ncut = (argc == 2) ? atoi (argv[1]) : 100000;
	
	chdir ("/home/aa4/gmt/wvs/pol");
	
	fp_bad = fopen ("/home/aa4/gmt/wvs/bad_3.lis", "w");
	fp_fix = fopen ("/home/aa4/gmt/wvs/fix_3.lis", "w");
	fp = fopen ("/home/aa4/gmt/wvs/headers2.b", "r");
	fp_in = fopen ("/home/aa4/gmt/wvs/final_x_polygons.b", "r");
	fread ((char *)&n_id, sizeof (int), 1, fp);
	fread ((char *)poly, sizeof (struct CHECK), n_id, fp);
	fclose (fp);
	
	i = 0;
	fpj = fopen ("/home/aa4/gmt/wvs/junk", "r");
	while (fgets (line, 512, fpj)) {
		sscanf (line, "%d %d", &go[0], &go[1]);
		w = (floor (poly[go[0]].h.west) - 1) * MILL;
		e = (ceil (poly[go[0]].h.east) + 1) * MILL;
		s = (floor (poly[go[0]].h.south) - 1) * MILL;
		n = (ceil (poly[go[0]].h.north) + 1) * MILL;
		for (j = 0; j < 2; j++) {
				
			sprintf (file, "polygon.%d\0", go[j]);
			fp = fopen (file, "w");
		
			for (id = 0; id < n_id && go[j] != poly[id].h.id; id++);
			
			fseek (fp_in, poly[id].pos, 0);
		
			for (k = 0; k < poly[id].h.n; k++) {
				if (fread((char *)&p, sizeof(struct LONGPAIR), 1, fp_in) != 1) {
					fprintf(stderr,"polygon_extract:  ERROR  reading file.\n");
					exit(-1);
				}
				if (p.y < s || p.y > n) continue;
				if (poly[id].h.greenwich && p.x > poly[id].h.datelon) p.x -= M360;
				if (p.x < w || p.x > e) continue;
			
				fprintf (fp, "%d\t%d\n", p.x, p.y);
			}
			fclose (fp);
		}
		
		sprintf (cmd, "XYX -- -X1.0e-6 -Y1.0e-6  polygon.%d polygon.%d\0", go[1], go[0]);
		system (cmd);
		printf ("(d)elete, (s)witch level to 2, (e)dit polygon %d (q = quit) [d]? ", go[0]);
		ss[0] = 0;
		gets (ss);
		if (ss[0] == '\0' || ss[0] == 'd') {	/* Remove from x.lis */
			fprintf (fp_bad, "%d\n", go[0]);
			fflush (fp_bad);
			printf ("Removing polygon # %d\n", go[1]);
		}
		else if (ss[0] == 's') {	/* Remove from x.lis */
			fprintf (fp_fix, "%d set level = 2\n", go[0]);
			fflush (fp_fix);
		}
		else if (ss[0] == 'q') {
			exit (0);
		}
		else if (ss[0] == 'e') {
			fprintf (fp_fix, "%d\n", go[0]);
			fflush (fp_fix);
		}
		
	} while (np);
	
	fclose (fpj);
	fclose (fp_in);
	fclose (fp_bad);
	fclose (fp_fix);
	
}
