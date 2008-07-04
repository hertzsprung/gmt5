/*
 *	$Id: polygon_findlevel.c,v 1.12 2008-07-04 01:06:47 guru Exp $
 */
#include "wvs.h"


#define ANTARCTICA 34	/* Current id of Antarctica in sorted list */

struct BLOB {
	struct GMT3_POLY h;
	int inside[6];
	int start;
	unsigned char n_inside, reverse;
	int x0, y0;
} blob[N_POLY];

int *lon, *lat;
double *flon, *flat;
struct LONGPAIR *pp;

int main (int argc, char **argv) {
	int i, j, k, n_id, pos, id, id1, id2, idmax, intest, sign, max_level, n, n_of_this[6];
	int n_reset = 0, old, bad = 0, ix0, off, set, fast = 0;
	double x0, west1, west2, east1, east2, size, val;
	FILE *fp, *fp2, *fpx;
	struct LONGPAIR p;
	char line[80];
	
	if (argc == 1) {
		fprintf (stderr, "usage: polygon_findlevel final_polygons.b revised_final_dbase.b [-s]\n");
		fprintf (stderr, "Note 1: assumes poly # 0,1,2 are Eurasia, Americas,Australia (unless -s)\n");
		fprintf (stderr, "Note 2: Will recalculate areas unless areas.lis already exists\n");
		exit (EXIT_FAILURE);
	}

	fast = (argc != 4);

	fpx = fopen ("still_bad.lis", "w");
	
	for (i = 0; i < 6; i++) n_of_this[i] = 0;
	fprintf (stderr, "Read headers\n");
	fp = fopen (argv[1], "r");
	
	/* Reset ids as we go along */
	
	n_id = pos = 0;
	while (pol_readheader (&blob[n_id].h, fp) == 1) {
		if (fabs (blob[n_id].h.east - blob[n_id].h.west) == 360.0) blob[n_id].h.south = -90.0;	/* Antarctica */
		pos += sizeof (struct GMT3_POLY);
		blob[n_id].start = pos;
		if (pol_fread (&p, 1, fp) != 1) {
			fprintf(stderr,"polygon_findlevel:  ERROR  reading file.\n");
			exit(-1);
		}
		blob[n_id].x0 = p.x;	/* Pick any point on the polygon */
		blob[n_id].y0 = p.y;
		blob[n_id].n_inside = 0;
		blob[n_id].reverse = 0;
		fseek (fp, (blob[n_id].h.n - 1) * sizeof(struct LONGPAIR), 1);
		pos += blob[n_id].h.n * sizeof(struct LONGPAIR);
		blob[n_id].h.id = n_id;
		if (blob[n_id].h.n < 3) blob[n_id].h.source = -1;
		n_id++;
	}
	
	if ((fp2 = fopen ("areas.lis", "r")) != NULL) {	/* Read areas etc */
		for (i = 0; i < n_id; i++) {
			fgets (line, 80, fp2);
			sscanf (line, "%d %lf", &j, &size);
			sign = (size < 0.0) ? -1 : 1;
			blob[i].reverse = sign + 1;
			blob[i].h.area = fabs(size);
			if (i != j) {
				fprintf (stderr, "Error reading areas\n");
				exit (EXIT_FAILURE);
			}
		}
	}
	else {

		fprintf (stderr, "\n\nFind area and direction of polygons\n");

		area_init ();

		flon = (double *) GMT_memory (CNULL, blob[0].h.n, sizeof(double), "polygon_findlevel");
		flat = (double *) GMT_memory (CNULL, blob[0].h.n, sizeof(double), "polygon_findlevel");

		fp2 = fopen ("areas.lis", "w");
	
		for (id = 0; id < n_id; id++) {
		
			if (blob[id].h.source == -1) continue;	/* Marked for deletion */
			
			fseek (fp, (long)blob[id].start, 0);
			for (k = 0; k < blob[id].h.n; k++) {
				if (pol_fread (&p, 1, fp) != 1) {
					fprintf(stderr,"polygon_findlevel:  ERROR  reading file.\n");
					exit(-1);
				}
				if (blob[id].h.greenwich && p.x > blob[id].h.datelon) p.x -= M360;
				flon[k] = p.x * 1.0e-6;
				flat[k] = p.y * 1.0e-6;
			}
			size = 1.0e-6 * area_size (flon, flat, blob[id].h.n, &sign); /* in km^2 */
			blob[id].h.area = size;
			blob[id].reverse = sign + 1;
			fprintf (fp2, "%d\t%g\n", id, size * sign);
		}

		free ((void *)flon);
		free ((void *)flat);
	}
	fclose (fp2);
	
	lon = (int *) GMT_memory (CNULL, N_LONGEST, sizeof(int), "polygon_findlevel");
	lat = (int *) GMT_memory (CNULL, N_LONGEST, sizeof(int), "polygon_findlevel");
	
	/* Scale crude polygons by 1e6 to match the data scale */
	
	for (i = 0; i < N_EUR_O; i++) {
		ieur_o[0][i] *= MILL;
		ieur_o[1][i] *= MILL;
	}
	for (i = 0; i < N_EUR_I; i++) {
		ieur_i[0][i] *= MILL;
		ieur_i[1][i] *= MILL;
	}
	for (i = 0; i < N_AFR_I; i++) {
		iafr_i[0][i] *= MILL;
		iafr_i[1][i] *= MILL;
	}
	for (i = 0; i < N_AM_O; i++) {
		iam_o[0][i] *= MILL;
		iam_o[1][i] *= MILL;
	}
	for (i = 0; i < N_SAM_I; i++) {
		isam_i[0][i] *= MILL;
		isam_i[1][i] *= MILL;
	}
	for (i = 0; i < N_NAM_I; i++) {
		inam_i[0][i] *= MILL;
		inam_i[1][i] *= MILL;
	}
	for (i = 0; i < N_AUS_O; i++) {
		iaus_o[0][i] *= MILL;
		iaus_o[1][i] *= MILL;
	}
	for (i = 0; i < N_AUS_I; i++) {
		iaus_i[0][i] *= MILL;
		iaus_i[1][i] *= MILL;
	}
	
	/* Test everything except Antarctica which has no lakes */
	
	fprintf (stderr, "Start inside testing\n\n");
	
	for (id1 = 0; id1 < n_id; id1++) {	/* For all anchor polygons */
	
		if (blob[id1].h.source == -1) continue;	/* Marked for deletion */
		
		if (fabs (blob[id1].h.east - blob[id1].h.west) == 360.0) continue;	/* But skip Antarctica since there are no lakes in the data set */
		
		if (id1%10 == 0) fprintf (stderr, "Polygon %d\r", id1);

		fseek (fp, (long)blob[id1].start, 0);
		for (k = 0; k < blob[id1].h.n; k++) {
			if (pol_fread (&p, 1, fp) != 1) {
				fprintf(stderr,"polygon_findlevel:  ERROR  reading file.\n");
				exit(-1);
			}
			if (blob[id1].h.greenwich && p.x > blob[id1].h.datelon) p.x -= M360;
			lon[k] = p.x;
			lat[k] = p.y;
		}
		n = blob[id1].h.n;
		
		west1 = blob[id1].h.west;	east1 = blob[id1].h.east;
		if (blob[id1].h.greenwich) {
			west1 += 360.0;
			east1 += 360.0;
		}
		
		for (id2 = 0; id2 < n_id; id2++) {
			
			if (fabs (blob[id2].h.east - blob[id2].h.west) == 360.0) continue;	/* But skip Antarctica */
			if (id1 == id2) continue;	/* Skip self testing */
			if (blob[id2].h.source == -1) continue;	/* Marked for deletion */
			
			/* First perform simple tests based on min/max coordinates */
			
			if ( (blob[id2].h.south > blob[id1].h.north) || (blob[id2].h.north < blob[id1].h.south)) continue;
			
			west2 = blob[id2].h.west;	east2 = blob[id2].h.east;
			off = 0.0;
			if (blob[id2].h.greenwich) {
				west2 += 360.0;
				east2 += 360.0;
				off = M360;
			}
			while (east2 < west1) {
				east2 += 360.0;
				west2 += 360.0;
				off += M360;
			}
			if ((west2 > east1) || (east2 < west1)) continue;	/* Clearly not overlapping */
			
			/* Must compare with polygon boundaries */
			
			if (fast && id1 == 0) {	/* Eurasia, first do quick coarse test */
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, ieur_o[0], ieur_o[1], N_EUR_O);
				if (!intest) continue;
				
				/* So point is inside crude outside. Now check if it is inside crude inside */
				
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, ieur_i[0], ieur_i[1], N_EUR_I);

				if (!intest) intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, iafr_i[0], iafr_i[1], N_AFR_I);

				if (intest == 2) {	/* way inside, set levels */
					blob[id2].inside[blob[id2].n_inside] = id1;
					blob[id2].n_inside++;
					if (blob[id2].n_inside == 6) {
						fprintf (stderr, "You're fucked again!\n");
						exit (EXIT_FAILURE);
					}
					continue;
				}
				/* If not, we fall down to the _real_ test */
			}
			else if (fast && id1 == 1) {	/* Americas, first do quick test */
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, iam_o[0], iam_o[1], N_AM_O);
				if (!intest) continue;

				
				/* So point is inside crude outside. Now check if it is inside crude inside */
				
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, isam_i[0], isam_i[1], N_SAM_I);

				if (!intest) intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, inam_i[0], inam_i[1], N_NAM_I);

				if (intest == 2) {	/* way inside, set levels */
					blob[id2].inside[blob[id2].n_inside] = id1;
					blob[id2].n_inside++;
					if (blob[id2].n_inside == 6) {
						fprintf (stderr, "You're fucked again!\n");
						exit (EXIT_FAILURE);
					}
					continue;
				}
				/* If not, we fall down to the _real_ test */
			}
			else if (fast && id1 == 3) {	/* Australia, first do quick test */
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, iaus_o[0], iaus_o[1], N_AUS_O);
				if (!intest) continue;
				
				/* So point is inside crude outside. Now check if it is inside crude inside */
				
				intest = non_zero_winding2 (blob[id2].x0 + off, blob[id2].y0, iaus_i[0], iaus_i[1], N_AUS_I);

				if (intest == 2) {	/* way inside, set levels */
					blob[id2].inside[blob[id2].n_inside] = id1;
					blob[id2].n_inside++;
					if (blob[id2].n_inside == 6) {
						fprintf (stderr, "You're fucked again!\n");
						exit (EXIT_FAILURE);
					}
					continue;
				}
				/* If not, we fall down to the _real_ test */
			}
			
			/* Here we need to perform complete inside test */
			
			x0 = blob[id2].x0 * 1.0e-6;
			if (id1 == 0 && x0 > blob[id1].h.east)
				x0 -= 360.0;
			else if (id1 > 0 && blob[id1].h.greenwich && x0 > 180.0)
				x0 -= 360.0;
			else if (!blob[id1].h.greenwich && x0 < 0.0)
				x0 += 360.0;
			
			if (!(lon[0] == lon[n-1] && lat[0] == lat[n-1])) {
				lon[n] = lon[0];
				lat[n] = lat[0];
				n++;
			}
			ix0 = x0 * MILL;
			intest = non_zero_winding2 (ix0, blob[id2].y0, lon, lat, n);
			
			if (!intest) continue;
			if (intest == 1) {
				set = FALSE;
				if (blob[id1].h.source == 0 && blob[id2].h.source == 0 && blob[id1].h.n == blob[id2].h.n) { /* duplicate */
					fprintf (fpx, "%d is duplicate of %d, %d removed\n", id2, id1, id2);
					set = TRUE;
				}
				else {
					fprintf (stderr, "Point on edge!, ids = %d and %d\n", id1, id2);
					fprintf (fpx, "Point on edge!, ids = %d and %d\n", id1, id2);
					bad++;
				}
				if (set) blob[id2].h.source = -1;
			}
			else if (blob[id2].h.source == 1) { /* WVS inside CIA or WVS! */
				if (blob[id1].h.source == 1 && blob[id2].h.n < blob[id1].h.n) {  /* small WVS inside another WVS - should be lake */
					if (blob[id2].h.level != 2) {
						fprintf (fpx, "%d is inside %d, %d reset to lake\n", id2, id1, id2);
						blob[id2].h.level = 2;
						fflush (fpx);
					}
				}
				else { /* WVS inside CIA, assume CIA is bad duplicate to be removed */
					fprintf (fpx, "%d is inside %d, %d deleted\n", id2, id1, id1);
					blob[id1].h.source = -1;
					fflush (fpx);
					bad++;
				}
			}
			else if (blob[id2].h.source == 0){	/* CIA inside WVS, check if bad duplicate */
				if ((val = blob[id2].h.area / blob[id1].h.area) > 0.95) { 
					fprintf (fpx, "%d is inside %d but has %.1f %% of area. %d deleted\n", id2, id1, val * 100.0, id2);
					blob[id2].h.source = -1;
					fflush (fpx);
					bad++;
				}
			}
			
			blob[id2].inside[blob[id2].n_inside] = id1;
			blob[id2].n_inside++;
			if (blob[id2].n_inside == 6) {
				fprintf (stderr, "You're fucked again!\n");
				exit (EXIT_FAILURE);
			}
		}
	}
		
	free ((void *)lon);
	free ((void *)lat);

	fprintf (stderr, "\nFound %d bad cases\n", bad);
	
	/* Check if polygons need to be reversed */
	
	/* Find levels and decide if polygon need to be reversed */
	
	fprintf (stderr, "Check for need to reverse polygons\n");
	
	max_level = idmax = 0;
	for (id = 0; id < n_id; id++) {
		if (blob[id].h.source == -1) continue;	/* Marked for deletion */
		old = blob[id].h.level;
		blob[id].h.level = blob[id].n_inside + 1;
		if (old > 0 && old != blob[id].h.level) n_reset++;
		n_of_this[blob[id].h.level]++;
		
		if (blob[id].h.level > max_level) {
			max_level = blob[id].h.level;
			idmax = id;
		}
		
		if ( (blob[id].h.level%2) && blob[id].reverse == 0)	/* Land and negative area */
			blob[id].reverse = 1;
		else if ( !(blob[id].h.level%2) && blob[id].reverse == 2)	/* Water and positive area */
			blob[id].reverse = 1;
		else
			blob[id].reverse = 0;
			
	}
	
	fprintf (fpx, "%d polygons had their presumed level reset\n", n_reset);
	fprintf (fpx, "max_level = %d for polygon %d (%d", max_level, idmax, idmax);
	for (i = 0; i < blob[idmax].n_inside; i++) fprintf (fpx, "-%d", blob[idmax].inside[i]);
	fprintf (fpx, ")\n");
	for (i = 1; i <= max_level; i++) fprintf (fpx, "Level%d: %d polygons\n", i, n_of_this[i]);
	fclose (fpx);
	
	fprintf (stderr, "Write out new data base\n");

	/* Write new base */
	
	pp = (struct LONGPAIR *) GMT_memory (CNULL, N_LONGEST, sizeof(struct LONGPAIR), "polygon_findlevel");
	
	fp2 = fopen (argv[2], "w");
	for (id = i = 0; id < n_id; id++) {
		if (blob[id].h.source == -1) continue;
		blob[id].h.id = i;
		pol_writeheader (&blob[id].h, fp2);
		fseek (fp, (long)blob[id].start, 0);
		if (pol_fread (pp, blob[id].h.n, fp) != blob[id].h.n) {
			fprintf(stderr,"polygon_findlevel:  ERROR  reading file.\n");
			exit(-1);
		}
		if (blob[id].reverse) {	/* Reverse polygon */
			fprintf (fpx, "Reversed polygon %d\n", id);
			for (j = blob[id].h.n - 1; j >= 0; j--) pol_fwrite (&pp[j], 1, fp2);
		}
		else
			pol_fwrite (pp, blob[id].h.n, fp2);
		i++;
	}
	
	fclose (fp);
	fclose (fp2);
	
	fp = fopen ("hierarchy.lis", "w");
	for (id = 0; id < n_id; id++) {
		fprintf (fp, "%d: Level %d :", blob[id].h.id, blob[id].n_inside+1);
		for (i = 0; i < blob[id].n_inside; i++) fprintf (fp, "\t%d", blob[id].inside[i]);
		fprintf (fp, "\n");
	}
	fclose (fp);
	free ((void *)pp);
	
	exit (EXIT_SUCCESS);
}	
