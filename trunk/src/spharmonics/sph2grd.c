/*--------------------------------------------------------------------
 *    $Id: sph2grd.c,v 1.12 2009-11-12 17:24:35 remko Exp $
 *
 *	Copyright (c) 1991-2009 by P. Wessel and W. H. F. Smith
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * sph2grd evalutes a grid using a spherical harmonics model
 *
 * Author:	Paul Wessel
 * Date:	1-JUN-2006
 */
 
#include "gmt.h"

struct SPH2GRD_CTRL {	/* All control options for this program (except common args) */
	/* active is TRUE if the option has been activated */
	struct D {	/* -D */
		BOOLEAN active;
		char mode;
	} D;
	struct E {	/* -E */
		BOOLEAN active;
	} E;
	struct F {	/* -F */
		BOOLEAN active;
	} F;
	struct G {	/* -G<grdfile> */
		BOOLEAN active;
		char *file;
	} G;
	struct I {	/* -Idx[/dy] */
		BOOLEAN active;
		double xinc, yinc;
	} I;
	struct L {	/* -L<lc>/<lp>/<hp>/<hc> or -L<lo>/<hi> */
		BOOLEAN active;
		int mode;
		double lc, lp, hp, hc;
	} L;
	struct N {	/* -Ng|m|s */
		BOOLEAN active;
		char mode;
	} N;
	struct Q {	/* -Q */
		BOOLEAN active;
	} Q;
};

int main (int argc, char **argv)
{
	GMT_LONG i, j, ij, n_expected_fields, n_fields, n_files = 0, f_arg, error = 0, n_read = 0;
	
	float *grd;
	
	double *in, *lon, lat;
	
	FILE *fp;
	
	struct GRD_HEADER header;
	struct SPH2GRD_CTRL *Ctrl;

	void *New_sph2grd_Ctrl (), Free_sph2grd_Ctrl (struct SPH2GRD_CTRL *C);

	argc = GMT_begin (argc, argv);

	Ctrl = (struct SPH2GRD_CTRL *) New_sph2grd_Ctrl ();		/* Allocate and initialize defaults in a new control structure */
	
	GMT_grd_init (&header, argc, argv, FALSE);

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				/* Common parameters */
                      
				case 'R':
  				case 'b':
				case '\0':
					error += GMT_parse_common_options (argv[i], &header.x_min, &header.x_max, &header.y_min, &header.y_max);
					break;

				/* Supplemental parameters */

				case 'D':	/* Evaluate derivative solutions */
					Ctrl->D.active = TRUE;
					Ctrl->D.mode = argv[i][2];
					break;
				case 'E':	/* Evaluate on ellipsoid */
					Ctrl->E.active = TRUE;
					break;
				case 'F':
					Ctrl->F.active = TRUE;
					break;
				case 'G':
					Ctrl->G.active = TRUE;
					Ctrl->G.file = strdup (&argv[i][2]);
					break;
				case 'I':
					Ctrl->I.active = TRUE;
					if (GMT_getinc (&argv[i][2], &Ctrl->I.xinc, &Ctrl->I.yinc)) {
						GMT_inc_syntax ('I', 1);
						error++;
					}
					break;
 				case 'L':	/* Bandpass or Gaussian filter */
					Ctrl->L.active = TRUE;
					sscanf (&argv[i][2], "%lg/%lg/%lg/%lg", &Ctrl->L.lc, &Ctrl->L.lp, &Ctrl->L.hp, &Ctrl->L.hc);
					break;
				case 'N':
					Ctrl->N.active = TRUE;
					Ctrl->N.mode = argv[i][2];
					break;
				case 'Q':
					Ctrl->Q.active = TRUE;
					break;
				default:
					error++;
					GMT_default_error (argv[i][1]);
					break;
			}
		}
		else {
			n_files++;
			f_arg = i;
		}
	}
		
	if (argc == 1 || GMT_give_synopsis_and_exit) {
		fprintf (stderr, "sph2grd %s - Evaluate spherical harmonic models on a grid\n\n", GMT_VERSION);
		fprintf (stderr, "usage: sph2grd [coeff_file] %s %s [-Dg|n]\n", GMT_I_OPT, GMT_Rgeo_OPT);
		fprintf (stderr, "\t[-E] [-F] [-G<grdfile>] [-L[d]<filter>] [-N<norm>] [-Q] [-V] [%s]\n\n", GMT_bi_OPT);
		if (GMT_give_synopsis_and_exit) exit (EXIT_FAILURE);
		fprintf (stderr, "	coeff_file (or stdin) contains records of degree, order, cos, sin\n");
		GMT_explain_option ('R');
		GMT_inc_syntax ('I', 0);
		fprintf (stderr, "\n\tOPTIONS:\n");
		fprintf (stderr, "\t-D Will evaluate a derived field from a geopotential model.  Choose between\n");
		fprintf (stderr, "\t   -Dg will compute the gravitational field [Add -E for anomalies on ellipsoid]\n");
		fprintf (stderr, "\t   -Dn will compute the geoid [Add -E for anomalies on ellipsoid]\n");
		fprintf (stderr, "\t-E to evaluate expansion on the current ellipsoid [Default is sphere]\n");
		fprintf (stderr, "\t-F Force pixel registration [Default is gridline registration].\n");
		fprintf (stderr, "\t-G filename for output grid file\n");
		fprintf (stderr, "\t-L Filter coefficients according to one of two kinds of filter specifications:.\n");
		fprintf (stderr, "\t   Use -Ld if values are given in terms of coefficient degrees [Default is km]\n");
		fprintf (stderr, "\t   a) Cosine band-pass: Append four wavelengths <lc>/<lp>/<hp>/<hc>.\n");
		fprintf (stderr, "\t      coefficients outside <lc>/<hc> are cut; inside <lp>/<hp> are passed, rest are tapered.\n");
		fprintf (stderr, "\t      Replace wavelength by - to skip, e.g.  -L-/-/500/100 is a low-pass filter.\n");
		fprintf (stderr, "\t   b) Gaussian band-pass: Append two wavelengths <lo>/<hi> where filter amplitudes = 0.5.\n");
		fprintf (stderr, "\t      Replace wavelength by - to skip, e.g.  -L300/- is a high-pass Gaussian filter.\n");
		fprintf (stderr, "\t-N Normalization used for coefficients.  Choose among\n");
		fprintf (stderr, "\t   m: Mathematical normalization - inner products summed over surface equal 1 [Default]\n");
		fprintf (stderr, "\t   g: Geodesy normalization - inner products summed over surface equal 4pi\n");
		fprintf (stderr, "\t   s: Schmidt normalization - as used in geomagnetism\n");
		fprintf (stderr, "\t-Q Coefficients have phase convention from physics, i.e., the (-1)^m factor\n");
		GMT_explain_option ('V');
		GMT_explain_option ('i');
		GMT_explain_option ('n');
		fprintf(stderr, "\t   Default is 4 columns\n");
		GMT_explain_option ('.');

		exit (EXIT_FAILURE);
	}
	
	GMT_check_lattice (&Ctrl->I.xinc, &Ctrl->I.yinc, &Ctrl->F.active, &Ctrl->I.active);

	if (n_files > 1) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR:  Can only handle one input coefficient file\n", GMT_program);
		error++;
	}
	if (!project_info.region_supplied) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR:  Must specify -R option\n", GMT_program);
		error++;
	}
	if (!Ctrl->G.file) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR option -G:  Must specify output file\n", GMT_program);
		error++;
	}
	if (Ctrl->D.active && !(Ctrl->D.mode == 'g' || Ctrl->D.mode == 'n')) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR option -D:  Must append g or n\n", GMT_program);
		error++;
	}
	if (!(Ctrl->N.mode == 'm' || Ctrl->N.mode == 'g' || Ctrl->N.mode == 's')) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR:  -N Normalization must be one of m, g, or s\n", GMT_program);
		error++;
	}
	if (Ctrl->I.xinc <= 0.0 || Ctrl->I.yinc <= 0.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -I option.  Must specify positive increment(s)\n", GMT_program);
		error++;
	}

	if (error) exit (EXIT_FAILURE);

	if (n_files == 1 && (fp = GMT_fopen (argv[f_arg], GMT_io.r_mode)) == NULL) {
		fprintf (stderr, "%s: Cannot open file %s\n", GMT_program, argv[f_arg]);
		exit (EXIT_FAILURE);
	}
	else {
		fp = GMT_stdin;
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
	}
	n_expected_fields = (GMT_io.binary[GMT_IN]) ? GMT_io.ncol[GMT_IN] : 4;
	while ((n_fields = GMT_input (fp, &n_expected_fields, &in)) >= 0 && !(GMT_io.status & GMT_IO_EOF)) {	/* Not yet EOF */
		n_read++;
		if (GMT_io.status & GMT_IO_MISMATCH) {
			fprintf (stderr, "%s: Mismatch between actual (%ld) and expected (%ld) fields near line %ld\n", GMT_program, n_fields, n_expected_fields, n_read);
			exit (EXIT_FAILURE);
		}
		/* Store coefficients somewhere */
	}
	if (fp != GMT_stdin) GMT_fclose (fp);
	
	header.x_inc = Ctrl->I.xinc;
	header.y_inc = Ctrl->I.yinc;
	header.node_offset = Ctrl->F.active;
	GMT_RI_prepare (&header);	/* Ensure -R -I consistency and set nx, ny */
	GMT_err_fail (GMT_grd_RI_verify (&header, 1), Ctrl->G.file);

	grd = (float *) GMT_memory (VNULL, (size_t)(header.nx * header.ny), sizeof (float), GMT_program);
	lon = (double *) GMT_memory (VNULL, (size_t)header.nx, sizeof (double), GMT_program);
	for (i = 0; i < header.nx; i++) lon[i] = GMT_i_to_x (i, header.x_min, header.x_max, header.x_inc, header.xy_off, header.nx);
		
	for (j = ij = 0; j < header.ny; j++) {
		lat = GMT_j_to_y (j, header.y_min, header.y_max, header.y_inc, header.xy_off, header.ny);
		if (gmtdefs.verbose) {
			fprintf (stderr, "Working on latitude: ");
			GMT_ascii_output_one (stderr, lat, 1);
			fprintf (stderr, "\r");
		}
		
		/* Compute the Legendre coefficients for this latitude */
			
		for (i = 0; i < header.nx; i++, ij++) grd[ij] = 0.0;
	}
	
	GMT_err_fail (GMT_write_grd (Ctrl->G.file, &header, grd, 0.0, 0.0, 0.0, 0.0, GMT_pad, FALSE), Ctrl->G.file);
	
	GMT_free ((void *)grd);
	GMT_free ((void *)lon);
	
	Free_sph2grd_Ctrl (Ctrl);	/* Deallocate control structure */

	GMT_end (argc, argv);

	exit (EXIT_SUCCESS);
}

void *New_sph2grd_Ctrl () {	/* Allocate and initialize a new control structure */
	struct SPH2GRD_CTRL *C;
	
	C = (struct SPH2GRD_CTRL *) GMT_memory (VNULL, 1, sizeof (struct SPH2GRD_CTRL), "New_sph2grd_Ctrl");
	
	C->N.mode = 'm';
	return ((void *)C);
}

void Free_sph2grd_Ctrl (struct SPH2GRD_CTRL *C) {	/* Deallocate control structure */
	if (C->G.file) GMT_free ((void *)C->G.file);	
	GMT_free ((void *)C);	
}
