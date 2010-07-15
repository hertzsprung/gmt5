/*
 *	$Id: polygon_to_shape.c,v 1.8 2010-07-15 23:14:12 guru Exp $
 * 
 *	Reads a polygon (or line) file and creates a multisegment GMT file with
 *	appropriate GIS tags so ogr2ogr can convert it to a shapefile.
 */

#include "gmt.h"
#include "wvs.h"

#define M270 270000000

struct POLYGON {
	struct GMT3_POLY h;
	struct	LONGPAIR *p;
} P[N_POLY];


int main (int argc, char **argv)
{
	FILE *fp_in, *fp;
	int n_id = 0, id, k, level, x, x0, y0, ymin = M90, ymax = -M90, hemi, first, lines = 0, n_levels, river, border;
	GMT_LONG np, nx;
	char file[BUFSIZ], cmd[BUFSIZ], *SRC[2] = {"WDBII", "WVS"}, *H = "EW", *ITEM[3] = {"polygon", "border", "river"};
	char *header[2] = {"# @VGMT­1.0 @GPOLYGON @Nid|level|source|parent_id|sibling_id|area @Tchar|integer|char|integer|integer|double\n",
		"# @VGMT­1.0 @GLINESTRING @Nid|level @Tchar|integer\n"};
	double area, *lon = NULL, *lat = NULL, *xx, *yy;
	EXTERN_MSC GMT_LONG GMT_wesn_clip (double *lon, double *lat, GMT_LONG n, double **x, double **y, GMT_LONG *total_nx);
        
	argc = GMT_begin (argc, argv);
	
	/* Set up some GMT variables needed later */
	project_info.degree[0] = project_info.degree[1] = TRUE;
	/* Supply dummy linear proj */
	project_info.projection = project_info.xyz_projection[0] = project_info.xyz_projection[1] = GMT_LINEAR;
	project_info.pars[0] = project_info.pars[1] = 1.0;
	GMT_err_fail (GMT_map_setup (-180.0, 180.0, -90.0, 90.0), "");
	
	if (argc < 2 || argc > 4) {
		fprintf (stderr,"usage:  polygon_to_shape file_res.b prefix [-L]\n");
		fprintf (stderr,"	file_res.b is the binary local file with all polygon info for a resolution\n");
		fprintf (stderr,"	prefix is used to form the files prefix_L[1-4].gmt\n");
		fprintf (stderr,"	These are then converted to shapefiles via ogr2ogr\n");
		fprintf (stderr,"	Append -b if file_res.b is a border line file\n");
		fprintf (stderr,"	Append -r if file_res.b is a river line file\n");
		exit (EXIT_FAILURE);
	}
	border = (argc == 4 && !strcmp (argv[3], "-b"));
	river  = (argc == 4 && !strcmp (argv[3], "-r"));
	lines = 2*river + border;	/* Since only one is set to 1 we get 0, 1, or 2 */
	fp_in = fopen (argv[1], "r");
		
	while (pol_readheader (&P[n_id].h, fp_in) == 1) {
		P[n_id].p = (struct LONGPAIR *) GMT_memory (VNULL, P[n_id].h.n, sizeof (struct LONGPAIR), "polygon_to_shape");
		if (pol_fread (P[n_id].p, P[n_id].h.n, fp_in) != P[n_id].h.n) {
			fprintf(stderr,"polygon_to_shape:  ERROR  reading file.\n");
			exit(-1);
		}
		for (k = 0; k < P[n_id].h.n; k++) {
			if (P[n_id].p[k].y < ymin) ymin = P[n_id].p[k].y;
			if (P[n_id].p[k].y > ymax) ymax = P[n_id].p[k].y;
		}
		n_id++;
	}
	fclose (fp_in);
	fprintf (stderr, "polygon_to_shape: Found %d %ss\n", n_id, ITEM[lines]);
	if (lines == 0) ymin = -90000000;	/* Because of Antarctica */
	n_levels = (lines == 0) ? 4 : 3;
	GMT_err_fail (GMT_map_setup (-180, 180, -90.0, 90.0), "");
	for (level = 1; level <= n_levels; level++) {	/* Make separate files for each level*/
		sprintf (file, "%s_L%d.gmt", argv[2], level);
		if ((fp = fopen (file, "w")) == NULL) {
			fprintf(stderr,"polygon_to_shape:  ERROR  creating file %s.\n", file);
			exit(-1);
		}
		fprintf (fp, "%s", header[lines]);
		fprintf (fp, "# @R-180/180/%.6f/%.6f @Jp\"+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs\"\n# FEATURE DATA\n", ymin*I_MILL, ymax*I_MILL);
		for (id = 0; id < n_id; id++) {
			if (P[id].h.level != level) continue;
			/* Here we found a polygon of the required level.  Write out polygon tag and info */
			area = P[id].h.area;
			if (P[id].h.river) area = -area;	/* Flag river lakes with negative area */
			if (P[id].h.id == 4 && !lines) {
				P[id].h.n--;	/* Skip the duplicate point */
				for (hemi = 0; hemi < 2; hemi++) {
					fprintf (fp, "> GSHHS polygon Id = %d-%c Level = %d Area = %.12g\n# @P @D%d-%c|%d|%s|%d|%d|%.12g\n",
						P[id].h.id, H[hemi], P[id].h.level, area, P[id].h.id, H[hemi], P[id].h.level, SRC[P[id].h.source], P[id].h.parent, P[id].h.ancestor, area);
					for (k = 0, first = TRUE; k < P[id].h.n; k++) {	/* Set up lons that go -20 to + 192 */
						if (hemi == 0) {
							if (P[id].p[k].x > M180) continue;
							x = P[id].p[k].x;
							if (first) {x0 = x; y0 = P[id].p[k].y;}
						}
						else if (hemi == 1) {
							if (P[id].p[k].x < M180) continue;
							x = P[id].p[k].x - M360;
							if (first) {x0 = x; y0 = P[id].p[k].y;}
						}
						fprintf (fp, "%.6f\t%.6f\n", x * I_MILL, P[id].p[k].y * I_MILL);
						first = FALSE;
					}
					x = (hemi == 0) ? 0 : -M180;
					fprintf (fp, "%.6f\t%.6f\n%.6f\t%.6f\n%.6f\t%.6f\n", x * I_MILL, -90.0, x0 * I_MILL, -90.0, x0 * I_MILL, y0 * I_MILL);
				}
				
			}
			else if (!lines && (P[id].h.west < 180.0 && P[id].h.east > 180.0)) {	/* Straddles dateline; must split into two parts thanx to GIS brilliance */
				lon = (double *)GMT_memory (VNULL, sizeof (double), P[id].h.n, GMT_program);
				lat = (double *)GMT_memory (VNULL, sizeof (double), P[id].h.n, GMT_program);
				for (k = 0; k < P[id].h.n; k++) {	/* Set up lons that go -20 to + 192 */
					x = (P[id].p[k].x > M270) ? P[id].p[k].x - M360 : P[id].p[k].x;
					lon[k] = x * I_MILL;	lat[k] = P[id].p[k].y * I_MILL;
				}
				for (hemi = 0; hemi < 2; hemi++) {
					if ((np = GMT_wesn_clip (lon, lat, P[id].h.n, &xx, &yy, &nx)) == 0) {
						fprintf (stderr, "%s: Error: Straddling 180 but not two parts?\n", GMT_program);
						continue;
					}
					fprintf (fp, "> GSHHS polygon Id = %d-%c Level = %d Area = %.12g\n# @P @D%d-%c|%d|%s|%d|%d|%.12g\n",
						P[id].h.id, H[hemi], P[id].h.level, area, P[id].h.id, H[hemi], P[id].h.level, SRC[P[id].h.source], P[id].h.parent, P[id].h.ancestor, area);
					for (k = 0; k < np; k++) GMT_xy_to_geo (&xx[k], &yy[k], xx[k], yy[k]);	/* Undo projection first */
					fprintf (fp, "%.6f\t%.6f\n", xx[0], yy[0]);
					for (k = 1; k < np; k++) {	/* Avoid printing zero increments */
						if (!(GMT_IS_ZERO (xx[k]-xx[k-1]) && GMT_IS_ZERO (yy[k]-yy[k-1]))) fprintf (fp, "%.6f\t%.6f\n", xx[k], yy[k]);
					}
					GMT_free ((void *)xx);	GMT_free ((void *)yy);
					for (k = 0; k < P[id].h.n; k++) lon[k] -= 360.0;	/* Set up lons that go -360 to -tiny */
				}
				GMT_free ((void *)lon);	GMT_free ((void *)lat);
			}
			else {	/* No problems, just write as is */
				if (lines)
					fprintf (fp, "> WDBII %s line Id = %d Level = %d\n# @D%d|%d\n", ITEM[lines], P[id].h.id, P[id].h.level, P[id].h.id, P[id].h.level);
				else {
					fprintf (fp, "> GSHHS polygon Id = %d Level = %d Area = %.12g\n# @P @D%d|%d|%s|%d|%d|%.12g\n",
					P[id].h.id, P[id].h.level, area, P[id].h.id, P[id].h.level, SRC[P[id].h.source], P[id].h.parent, P[id].h.ancestor, area);
				}
				for (k = 0; k < P[id].h.n; k++) {
					x = (P[id].p[k].x > P[id].h.datelon) ? P[id].p[k].x - M360 : P[id].p[k].x;
					fprintf (fp, "%.6f\t%.6f\n", x * I_MILL, P[id].p[k].y * I_MILL);
				}
			}
		}
		fclose (fp);	/* Done with this set */
	}
	
	for (id = 0; id < n_id; id++) GMT_free ((void *)P[id].p);
	
	fprintf (stderr,"Now convert to ESRI Shapefiles: ");
	for (level = 1; level <= n_levels; level++) {	/* Make separate files for each level*/
		sprintf (cmd, "ogr2ogr -f \"ESRI Shapefile\" %s_L%d %s_L%d.gmt\n", argv[2], level, argv[2], level);
		system (cmd);
		fprintf (stderr, "%d", level);
	}
	fprintf (stderr," done\nThe shapefiles will be in directories %s_L[1-%d]\n", argv[2], n_levels);
	
	GMT_end (argc, argv);
	
	exit (EXIT_SUCCESS);
}
