/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2013 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation; version 3 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 *  Brief synopsis: grdfft_new.c is a program to do various operations on
 *  grid files in the frequency domain.
 *
 * Author:	W.H.F. Smith
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 */

#define THIS_MODULE k_mod_grdfft_new /* I am grdfft_new */

#include "gmt.h"

#ifdef DEBUG
bool show_n_new = false;
#endif

struct GRDFFT_CTRL {
	unsigned int n_op_count, n_par;
	int *operation;
	double *par;

	struct In {
		bool active;
		unsigned int n_grids;	/* 1 or 2 */
		char *file[2];
	} In;
	struct A {	/* -A<azimuth> */
		bool active;
		double value;
	} A;
	struct C {	/* -C<zlevel> */
		bool active;
		double value;
	} C;
	struct D {	/* -D[<scale>|g] */
		bool active;
		double value;
	} D;
	struct E {	/* -E[r|x|y][w[k]] */
		bool active;
		bool give_wavelength;
		bool km;
		int mode;	/*-1/0/+1 */
	} E;
	struct F {	/* -F[x_or_y]<lc>/<lp>/<hp>/<hc> or -F[x_or_y]<lo>/<hi> */
		bool active;
		double lc, lp, hp, hc;
	} F;
	struct G {	/* -G<outfile> */
		bool active;
		char *file;
	} G;
	struct I {	/* -I[<scale>|g] */
		bool active;
		double value;
	} I;
	struct L {	/* -L[m|h] */
		bool active;
		bool debug;
		unsigned int mode;
	} L;
	struct N {	/* -N<stuff> */
		bool active;
		struct GMT_FFT_INFO info;
	} N;
	struct Q {	/* -Q[<prefix>] */
		bool active;
		char *prefix;
	} Q;
	struct S {	/* -S<scale> */
		bool active;
		double scale;
	} S;
#ifdef GMT_COMPAT	/* Now in gravfft in potential supplement */
	struct T {	/* -T<te/rl/rm/rw/ri> */
		bool active;
		double te, rhol, rhom, rhow, rhoi;
	} T;
#endif
	struct Z {	/* -Z[p] */
		bool active;
		unsigned int mode;
	} Z;
};

#define UP_DOWN_CONTINUE	0
#define AZIMUTHAL_DERIVATIVE	1
#define DIFFERENTIATE		2
#define INTEGRATE		3
#define ISOSTASY		4
#define FILTER_EXP		5
#define FILTER_BW		6
#define FILTER_COS		7
#define SPECTRUM		8

#define	MGAL_AT_45	980619.9203 	/* Moritz's 1980 IGF value for gravity in mGal at 45 degrees latitude */

struct F_INFO {
	double lc[3];		/* Low-cut frequency for r, x, and y	*/
	double lp[3];		/* Low-pass frequency for r, x, and y	*/
	double hp[3];		/* High-pass frequency for r, x, and y	*/
	double hc[3];		/* High-cut frequency for r, x, and y	*/
	double ltaper[3];	/* Low taper width for r, x, and y	*/
	double htaper[3];	/* High taper width for r, x, and y	*/
	double llambda[3];	/* Low full-wavelength where Gauss amp = 0.5 for r, x, and y	*/
	double hlambda[3];	/* High full-wavelength where Gauss amp = 0.5  for r, x, and y	*/
	double bw_order;	/* Order, N, of Butterworth filter	*/
	double (*filter) (struct F_INFO *, double, int);	/* Points to the correct filter function */

	bool set_already;	/* true if we already filled in the structure */
	unsigned int k_type;	/* Which of the x, y, or r wavenumbers we need */
	unsigned int kind;	/* FILTER_EXP, FILTER_BW, FILTER_COS  */
	unsigned int arg;	/* 0 = Gaussian, 1 = Butterworth, 2 = cosine taper,  */
};

void *New_grdfft_new_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDFFT_CTRL *C = NULL;

	C = GMT_memory (GMT, NULL, 1, struct GRDFFT_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	C->S.scale = 1.0;
	C->N.info.taper_width = 100.0;				/* Taper over entire margin strip by default */
	C->N.info.taper_mode = GMT_FFT_EXTEND_POINT_SYMMETRY;	/* Default action is edge-point symmetry */
	C->Q.prefix = strdup ("tapered");		/* Default prefix */
	return (C);
}

void Free_grdfft_new_Ctrl (struct GMT_CTRL *GMT, struct GRDFFT_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->operation) GMT_free (GMT, C->operation);	
	if (C->par) GMT_free (GMT, C->par);	
	if (C->In.file[0]) free (C->In.file[0]);	
	if (C->In.file[1]) free (C->In.file[1]);	
	if (C->G.file) free (C->G.file);	
	if (C->Q.prefix) free (C->Q.prefix);	
	GMT_free (GMT, C);	
}

unsigned int do_differentiate_new (struct GMT_GRID *Grid, double *par, struct GMT_FFT_WAVENUMBER *K)
{
	uint64_t k;
	double scale, fact;
	float *datac = Grid->data;

	/* Differentiate in frequency domain by multiplying by kr [scale optional] */

	scale = (*par != 0.0) ? *par : 1.0;
	datac[0] = datac[1] = 0.0;	/* Derivative of the mean is zero */
	for (k = 2; k < Grid->header->size; k += 2) {
		fact = scale * GMT_fft_get_wave (k, K);
		datac[k]   *= (float)fact;
		datac[k+1] *= (float)fact;
	}
	return (1);	/* Number of parameters used */
}

unsigned int do_integrate_new (struct GMT_GRID *Grid, double *par, struct GMT_FFT_WAVENUMBER *K)
{
	/* Integrate in frequency domain by dividing by kr [scale optional] */
	uint64_t k;
	double fact, scale;
	float *datac = Grid->data;

	scale = (*par != 0.0) ? *par : 1.0;
	datac[0] = datac[1] = 0.0;
	for (k = 2; k < Grid->header->size; k += 2) {
		fact = 1.0 / (scale * GMT_fft_get_wave (k, K));
		datac[k]   *= (float)fact;
		datac[k+1] *= (float)fact;
	}
	return (1);	/* Number of parameters used */
}

unsigned int do_continuation_new (struct GMT_GRID *Grid, double *zlevel, struct GMT_FFT_WAVENUMBER *K)
{
	uint64_t k;
	float tmp, *datac = Grid->data;

	/* If z is positive, the field will be upward continued using exp[- k z].  */

	for (k = 2; k < Grid->header->size; k += 2) {
		tmp = (float)exp (-(*zlevel) * GMT_fft_get_wave (k, K));
		datac[k]   *= tmp;
		datac[k+1] *= tmp;
	}
	return (1);	/* Number of parameters used */
}

unsigned int do_azimuthal_derivative_new (struct GMT_GRID *Grid, double *azim, struct GMT_FFT_WAVENUMBER *K)
{
	uint64_t k;
	float tempr, tempi, fact, *datac = Grid->data;
	double cos_azim, sin_azim;

	sincosd (*azim, &sin_azim, &cos_azim);

	datac[0] = datac[1] = 0.0;
	for (k = 2; k < Grid->header->size; k += 2) {
		fact = (float)(sin_azim * GMT_fft_any_wave (k, GMT_FFT_K_IS_KX, K) + cos_azim * GMT_fft_any_wave (k, GMT_FFT_K_IS_KY, K));
		tempr = -(datac[k+1] * fact);
		tempi = (datac[k] * fact);
		datac[k]   = tempr;
		datac[k+1] = tempi;
	}
	return (1);	/* Number of parameters used */
}

#ifdef GMT_COMPAT
#define	POISSONS_RATIO	0.25
#define	YOUNGS_MODULUS	1.0e11		/* Pascal = Nt/m**2  */
#define	NORMAL_GRAVITY	9.806199203	/* m/s**2  */

unsigned int do_isostasy_new (struct GMT_GRID *Grid, struct GRDFFT_CTRL *Ctrl, double *par, struct GMT_FFT_WAVENUMBER *K)
{

	/* Do the isostatic response function convolution in the Freq domain.
	All units assumed to be in SI (that is kx, ky, modk wavenumbers in m**-1,
	densities in kg/m**3, Te in m, etc.
	rw, the water density, is used to set the Airy ratio and the restoring
	force on the plate (rm - ri)*gravity if ri = rw; so use zero for topo in air.  */
	uint64_t k;
	double airy_ratio, rigidity_d, d_over_restoring_force, mk, k2, k4, transfer_fn;

	double te;	/* Elastic thickness, SI units (m)  */
	double rl;	/* Load density, SI units  */
	double rm;	/* Mantle density, SI units  */
	double rw;	/* Water density, SI units  */
	double ri;	/* Infill density, SI units  */

	float *datac = Grid->data;

	te = par[0];	rl = par[1];	rm = par[2];	rw = par[3];	ri = par[4];
	airy_ratio = -(rl - rw)/(rm - ri);

	if (te == 0.0) {	/* Airy isostasy; scale variable Ctrl->S.scale and return */
		Ctrl->S.scale *= airy_ratio;
		return (5);	/* Number of parameters used */
	}

	rigidity_d = (YOUNGS_MODULUS * pow (te, 3.0)) / (12.0 * (1.0 - POISSONS_RATIO * POISSONS_RATIO));
	d_over_restoring_force = rigidity_d / ((rm - ri) * NORMAL_GRAVITY);

	for (k = 0; k < Grid->header->size; k += 2) {
		mk = GMT_fft_get_wave (k, K);
		k2 = mk * mk;
		k4 = k2 * k2;
		transfer_fn = airy_ratio / ((d_over_restoring_force * k4) + 1.0);
		datac[k]   *= (float)transfer_fn;
		datac[k+1] *= (float)transfer_fn;
	}
	return (5);	/* Number of parameters used */
}
#endif

#ifndef M_LN2
#define M_LN2			0.69314718055994530942  /* log_e 2 */
#endif

double gauss_weight_new (struct F_INFO *f_info, double freq, int j) {
	double hi, lo;
	lo = (f_info->llambda[j] == -1.0) ? 0.0 : exp (-M_LN2 * pow (freq * f_info->llambda[j], 2.0));	/* Low-pass part */
	hi = (f_info->hlambda[j] == -1.0) ? 1.0 : exp (-M_LN2 * pow (freq * f_info->hlambda[j], 2.0));	/* Hi-pass given by its complementary low-pass */
	return (hi - lo);
}

double bw_weight_new (struct F_INFO *f_info, double freq, int j) {	/* Butterworth filter */
	double hi, lo;
	lo = (f_info->llambda[j] == -1.0) ? 0.0 : sqrt (1.0 / (1.0 + pow (freq * f_info->llambda[j], f_info->bw_order)));	/* Low-pass part */
	hi = (f_info->hlambda[j] == -1.0) ? 1.0 : sqrt (1.0 / (1.0 + pow (freq * f_info->hlambda[j], f_info->bw_order)));	/* Hi-pass given by its complementary low-pass */
	return (hi - lo);
}

double cosine_weight_grdfft_new (struct F_INFO *f_info, double freq, int j) {
	if (freq <= f_info->lc[j] || freq >= f_info->hc[j]) return(0.0);	/* In fully cut range.  Weight is zero.  */
	if (freq > f_info->lc[j] && freq < f_info->lp[j]) return (0.5 * (1.0 + cos (M_PI * (freq - f_info->lp[j]) * f_info->ltaper[j])));
	if (freq > f_info->hp[j] && freq < f_info->hc[j]) return (0.5 * (1.0 + cos (M_PI * (freq - f_info->hp[j]) * f_info->htaper[j])));
	return (1.0);	/* Freq is in the fully passed range, so weight is multiplied by 1.0  */
}

double get_filter_weight_new (uint64_t k, struct F_INFO *f_info, struct GMT_FFT_WAVENUMBER *K)
{
	double freq, return_value;

	freq = GMT_fft_any_wave (k, f_info->k_type, K);
	return_value = f_info->filter (f_info, freq, f_info->k_type);

	return (return_value);
}

void do_filter_new (struct GMT_GRID *Grid, struct F_INFO *f_info, struct GMT_FFT_WAVENUMBER *K)
{
	uint64_t k;
	float weight, *datac = Grid->data;

	for (k = 0; k < Grid->header->size; k += 2) {
		weight = (float) get_filter_weight_new (k, f_info, K);
		datac[k]   *= weight;
		datac[k+1] *= weight;
	}
}

unsigned int count_slashes_new (char *txt)
{
	unsigned int i, n;
	for (i = n = 0; txt[i]; i++) if (txt[i] == '/') n++;
	return (n);
}

bool parse_f_string_new (struct GMT_CTRL *GMT, struct F_INFO *f_info, char *c)
{
	unsigned int i, j, n_tokens, pos;
	bool descending;
	double fourvals[4];
	char line[GMT_TEXT_LEN256], p[GMT_TEXT_LEN256];

	/* Syntax is either -F[r|x|y]lc/hc/lp/hp (Cosine taper), -F[r|x|y]lo/hi (Gaussian), or -F[r|x|y]lo/hi/order (Butterworth) */

	strncpy (line, c,  GMT_TEXT_LEN256);
	i =  0;
	f_info->k_type = GMT_FFT_K_IS_KR;	/* j is Filter type: r=0, x=1, y=2  */

	if (line[i] == 'r') {
		i++;
		f_info->k_type = GMT_FFT_K_IS_KR;
	}
	else if (line[i] == 'x') {
		i++;
		f_info->k_type = GMT_FFT_K_IS_KX;
	}
	else if (line[i] == 'y') {
		i++;
		f_info->k_type = GMT_FFT_K_IS_KY;
	}
	fourvals[0] = fourvals[1] = fourvals[2] = fourvals[3] = -1.0;

	n_tokens = pos = 0;
	while ((GMT_strtok (&line[i], "/", &pos, p))) {
		if (n_tokens > 3) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Too many slashes in -F.\n");
			return (true);
		}
		if(p[0] == '-')
			fourvals[n_tokens] = -1.0;
		else {
			if ((sscanf(p, "%lf", &fourvals[n_tokens])) != 1) {
				GMT_report (GMT, GMT_MSG_NORMAL, " Cannot read token %d.\n", n_tokens);
				return (true);
			}
		}
		n_tokens++;
	}

	if (!(n_tokens == 2 || n_tokens == 3 || n_tokens == 4)) {
		GMT_report (GMT, GMT_MSG_NORMAL, "-F Cannot find 2-4 tokens separated by slashes.\n");
		return (true);
	}
	descending = true;
	if (f_info->kind == FILTER_BW && n_tokens == 3) n_tokens = 2;	/* So we dont check the order as a wavelength */

	for (i = 1; i < n_tokens; i++) {
		if (fourvals[i] == -1.0 || fourvals[i-1] == -1.0) continue;
		if (fourvals[i] > fourvals[i-1]) descending = false;
	}
	if (!(descending)) {
		GMT_report (GMT, GMT_MSG_NORMAL, "-F Wavelengths are not in descending order.\n");
		return (true);
	}
	j = f_info->k_type;
	if (f_info->kind == FILTER_COS) {	/* Cosine band-pass specification */
		if ((fourvals[0] * fourvals[1]) < 0.0 || (fourvals[2] * fourvals[3]) < 0.0) {
			GMT_report (GMT, GMT_MSG_NORMAL, "-F Pass/Cut specification error.\n");
			return (true);
		}

		/* Now everything is OK  */

		if (fourvals[0] >= 0.0 || fourvals[1] >= 0.0) {	/* Lower end values are set  */
			f_info->lc[j] = (2.0 * M_PI)/fourvals[0];
			f_info->lp[j] = (2.0 * M_PI)/fourvals[1];
			if (fourvals[0] != fourvals[1]) f_info->ltaper[j] = 1.0/(f_info->lc[j] - f_info->lp[j]);
		}

		if (fourvals[2] >= 0.0 || fourvals[3] >= 0.0) {	/* Higher end values are set  */
			f_info->hp[j] = (2.0 * M_PI)/fourvals[2];
			f_info->hc[j] = (2.0 * M_PI)/fourvals[3];
			if (fourvals[2] != fourvals[3]) f_info->htaper[j] = 1.0/(f_info->hc[j] - f_info->hp[j]);
		}
		f_info->filter = &cosine_weight_grdfft_new;
	}
	else if (f_info->kind == FILTER_BW) {	/* Butterworth specification */
		f_info->llambda[j] = (fourvals[0] == -1.0) ? -1.0 : fourvals[0] / TWO_PI;	/* TWO_PI is used to counteract the 2*pi in the wavenumber */
		f_info->hlambda[j] = (fourvals[1] == -1.0) ? -1.0 : fourvals[1] / TWO_PI;
		f_info->bw_order = 2.0 * fourvals[2];
		f_info->filter = &bw_weight_new;
	}
	else {	/* Gaussian half-amp specifications */
		f_info->llambda[j] = (fourvals[0] == -1.0) ? -1.0 : fourvals[0] / TWO_PI;	/* TWO_PI is used to counteract the 2*pi in the wavenumber */
		f_info->hlambda[j] = (fourvals[1] == -1.0) ? -1.0 : fourvals[1] / TWO_PI;
		f_info->filter = &gauss_weight_new;
	}
	f_info->arg = f_info->kind - FILTER_EXP;
	return (false);
}

int do_spectrum_new (struct GMT_CTRL *GMT, struct GMT_GRID *GridX, struct GMT_GRID *GridY, double *par, bool give_wavelength, bool km, char *file, struct GMT_FFT_WAVENUMBER *K)
{
	/* Compute cross-spectral estimates from the two grids Xand Y and return frequency f and 8 quantities:
	 * Xpower[f], Ypower[f], coherent power[f], noise power[f], phase[f], admittance[f], gain[f], coherency[f].
	 * Each quantity comes with its own 1-std dev error estimate, hence output is 17 columns.
	 * Equations based on spectrum1d.c */

	uint64_t dim[4] = {1, 1, 17, 0};	/* One table and one segment, with 1 + 2*8 = 17 columns and yet unknown rows */
	uint64_t k, nk, ifreq, *nused = NULL;
	char header[GMT_BUFSIZ], *name[2] = {"freq", "wlength"};
	unsigned int col;
	float *X = GridX->data, *Y = NULL;	/* Short-hands */
	double delta_k, r_delta_k, freq, coh_k, sq_norm, powfactor, tmp, eps_pow;
	double *X_pow = NULL, *Y_pow = NULL, *co_spec = NULL, *quad_spec = NULL;
	struct GMT_DATASET *D = NULL;
	struct GMT_DATASEGMENT *S = NULL;

	dim[2] = (GridY) ? 17 : 3;	/* Either 3 or 17 estimates */
#ifdef DEBUG
	if (show_n_new) dim[2]++;		/* Also write out nused[k] */
#endif
	if (*par > 0.0) {	/* X spectrum desired  */
		delta_k = K->delta_kx;	nk = K->nx2 / 2;
		GMT_fft_set_wave (GMT, GMT_FFT_K_IS_KX, K);
	}
	else if (*par < 0.0) {	/* Y spectrum desired  */
		delta_k = K->delta_ky;	nk = K->ny2 / 2;
		GMT_fft_set_wave (GMT, GMT_FFT_K_IS_KY, K);
	}
	else {	/* R spectrum desired  */
		if (K->delta_kx < K->delta_ky) {
			delta_k = K->delta_kx;	nk = K->nx2 / 2;
		}
		else {
			delta_k = K->delta_ky;	nk = K->ny2 / 2;
		}
		GMT_fft_set_wave (GMT, GMT_FFT_K_IS_KR, K);
	}

	/* Get arrays for summing stuff */
	X_pow     = GMT_memory (GMT, NULL, nk, double );
	nused     = GMT_memory (GMT, NULL, nk, uint64_t);
	if (GridY) {	/* For cross-spectral estimates */
		Y = GridY->data;	/* Shorthand for Y data */
		Y_pow     = GMT_memory (GMT, NULL, nk, double);
		co_spec   = GMT_memory (GMT, NULL, nk, double);
		quad_spec = GMT_memory (GMT, NULL, nk, double);
	}

	/* Loop over it all, summing and storing, checking range for r */

	r_delta_k = 1.0 / delta_k;

	for (k = 2; k < GridX->header->size; k += 2) {
		freq = GMT_fft_get_wave (k, K);
		ifreq = lrint (fabs (freq) * r_delta_k);	/* Smallest value returned might be 0 when doing r spectrum*/
		if (ifreq > 0) --ifreq;
		if (ifreq >= nk) continue;	/* Might happen when doing r spectrum  */
		X_pow[ifreq]     += (X[k]   * X[k] + X[k+1] * X[k+1]);	/* X x X* = Power of grid X */
		nused[ifreq]++;
		if (GridY) {	/* For cross-spectral estimates */
			Y_pow[ifreq]     += (Y[k]   * Y[k] + Y[k+1] * Y[k+1]);	/* Y x Y* = Power of grid Y */
			co_spec[ifreq]   += (Y[k]   * X[k] + Y[k+1] * X[k+1]);	/* Real part of Y x X* */
			quad_spec[ifreq] += (X[k+1] * Y[k] - Y[k+1] * X[k]);	/* Imag part of Y x X* */
		}
	}

	GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_FLOAT;	/* To counter-act any -fg setting */
	
	delta_k /= (2.0 * M_PI);	/* Write out frequency, not wavenumber  */
	powfactor = 4.0 / pow ((double)GridX->header->nm, 2.0);	/* Squared normalization of FFT */
	dim[3] = nk;
	if ((D = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim)) == NULL) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Unable to create a data set for spectral estimates\n");
		return (GMT->parent->error);
	}
	S = D->table[0]->segment[0];	/* Only one table with one segment here, with 17 cols and nk rows */
	if (give_wavelength && km) delta_k *= 1000.0;	/* Wanted distances measured in km */
	
	for (k = 0; k < nk; k++) {
		eps_pow = 1.0 / sqrt (nused[k]);	/* Multiplicative error bars for power spectra  */
		freq = (k + 1) * delta_k;
		if (give_wavelength) freq = 1.0 / freq;
		
		X_pow[k] *= powfactor;
		
		col = 0;
		/* Col 0 is the frequency (or wavelength) */
		S->coord[col++][k] = freq;
		/* Cols 1-2 are xpower and std.err estimate */
		S->coord[col++][k] = X_pow[k];
		S->coord[col++][k] = X_pow[k] * eps_pow;
		
		if (!GridY) {
#ifdef DEBUG
			if (show_n_new) S->coord[col][k] = nused[k];
#endif
			continue;
		}
		
		Y_pow[k] *= powfactor;
		co_spec[k] *= powfactor;
		quad_spec[k] *= powfactor;
		/* Compute coherence first since it is needed by many of the other estimates */
		coh_k = (co_spec[k] * co_spec[k] + quad_spec[k] * quad_spec[k]) / (X_pow[k] * Y_pow[k]);
		sq_norm = sqrt ((1.0 - coh_k) / (2.0 * coh_k));	/* Save repetitive expression further down */
		/* Cols 3-4 are ypower and std.err estimate */
		S->coord[col++][k] = Y_pow[k];
		S->coord[col++][k] = Y_pow[k] * eps_pow;
		/* Cols 5-6 are coherent power and std.err estimate */
		S->coord[col++][k] = tmp = Y_pow[k] * coh_k;
		S->coord[col++][k] = tmp * eps_pow * sqrt ((2.0 - coh_k) / coh_k);
		/* Cols 7-8 are noise power and std.err estimate */
		S->coord[col++][k] = tmp = Y_pow[k] * (1.0 - coh_k);
		S->coord[col++][k] = tmp * eps_pow;
		/* Cols 9-10 are phase and std.err estimate */
		S->coord[col++][k] = tmp = d_atan2 (quad_spec[k], co_spec[k]);
		S->coord[col++][k] = tmp * eps_pow * sq_norm;
		/* Cols 11-12 are admittance and std.err estimate */
		S->coord[col++][k] = tmp = co_spec[k] / X_pow[k];
		S->coord[col++][k] = tmp * eps_pow * fabs (sq_norm);
		/* Cols 13-14 are gain and std.err estimate */
		S->coord[col++][k] = tmp = sqrt (quad_spec[k]) / X_pow[k];
		S->coord[col++][k] = tmp * eps_pow * sq_norm;
		/* Cols 15-16 are coherency and std.err estimate */
		S->coord[col++][k] = coh_k;
		S->coord[col++][k] = coh_k * eps_pow * (1.0 - coh_k) * sqrt (2.0 / coh_k);
#ifdef DEBUG
		if (show_n_new) S->coord[col][k] = nused[k];
#endif
	}
	if (GridY) {
		sprintf (header, "#%s[0]\txpow[1]\tstd_xpow[2]\typow[3]\tstd_ypow[4]\tcpow[5]\tstd_cpow[6]\tnpow[7]\tstd_npow[8]\t" \
		"phase[9]\tstd_phase[10]\tadm[11]\tstd_ad[12]\tgain[13]\tstd_gain[14]\tcoh[15]\tstd_coh[16]", name[give_wavelength]);
		GMT_free (GMT, Y_pow);
		GMT_free (GMT, co_spec);
		GMT_free (GMT, quad_spec);
	}
	else
		sprintf (header, "#%s[0]\tpow[1]\tstd_pow[2]", name[give_wavelength]);
		
	D->table[0]->header = GMT_memory (GMT, NULL, 1, char *);
	D->table[0]->header[0] = strdup (header);
	D->table[0]->n_headers = 1;
	if (GMT_Write_Data (GMT->parent, GMT_IS_DATASET, GMT_IS_STREAM, GMT_IS_POINT, GMT_WRITE_SET, NULL, file, D) != GMT_OK) {
		return (GMT->parent->error);
	}
	if (GMT_Destroy_Data (GMT->parent, GMT_ALLOCATED, &D) != GMT_OK) {
		return (GMT->parent->error);
	}
	GMT_free (GMT, X_pow);
	GMT_free (GMT, nused);
	return (1);	/* Number of parameters used */
}

int GMT_grdfft_new_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdfft_new <ingrid> [<ingrid2>]  [-G<outgrid>|<table>] [-A<azimuth>] [-C<zlevel>]\n");
	GMT_message (GMT, "\t[-D[<scale>|g]] [-E[r|x|y][w[k]] [-F[x|y]<parameters>] [-I[<scale>|g]] [-L[m|h]]\n");
	GMT_message (GMT, "\t[-N[f|q|<nx>/<ny>][+e|m|n][+t<width>]] [-Q[<prefix>]] [-S<scale>]\n");
	GMT_message (GMT, "\t[-T<te>/<rl>/<rm>/<rw>/<ri>] [-Z[p]] [%s] [%s] [-ho]\n\n", GMT_V_OPT, GMT_f_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is the input grid file.\n");
	GMT_message (GMT, "\tOPTIONS:\n");
	GMT_message (GMT, "\t-G filename for output netCDF grid file OR 1-D spectrum (see -E).\n");
	GMT_message (GMT, "\t-A Take azimuthal derivative along line <azimuth> degrees CW from North.\n");
	GMT_message (GMT, "\t-C Continue field upward (+) or downward (-) to <zlevel> (meters).\n");
	GMT_message (GMT, "\t-D Differentiate, i.e., multiply by kr [ * scale].  Use -Dg to get mGal from m].\n");
	GMT_message (GMT, "\t-E Estimate spEctrum in the radial [Default], x, or y-direction.\n");
	GMT_message (GMT, "\t   Given one grid X, write f, Xpower[f] to output file (see -G) or stdout.\n");
	GMT_message (GMT, "\t   Given two grids X and Y, write f, Xpower[f], Ypower[f], coherent power[f],\n");
	GMT_message (GMT, "\t   noise power[f], phase[f], admittance[f], gain[f], coherency[f].\n");
	GMT_message (GMT, "\t   Each quantity is followed by a column of 1 std dev. error estimates.\n");
	GMT_message (GMT, "\t   Append w to write wavelength instead of frequency; append k to report\n");
	GMT_message (GMT, "\t   wavelength in km (geographic grids only) [m].\n");
	GMT_message (GMT, "\t-F Filter r [x] [y] freq according to one of three kinds of filter specifications:\n");
	GMT_message (GMT, "\t   a) Cosine band-pass: Append four wavelengths <lc>/<lp>/<hp>/<hc>.\n");
	GMT_message (GMT, "\t      freq outside <lc>/<hc> are cut; inside <lp>/<hp> are passed, rest are tapered.\n");
	GMT_message (GMT, "\t      Replace wavelength by - to skip, e.g., -F-/-/500/100 is a low-pass filter.\n");
	GMT_message (GMT, "\t   b) Gaussian band-pass: Append two wavelengths <lo>/<hi> where filter amplitudes = 0.5.\n");
	GMT_message (GMT, "\t      Replace wavelength by - to skip, e.g., -F300/- is a high-pass Gaussian filter.\n");
	GMT_message (GMT, "\t   c) Butterworth band-pass: Append two wavelengths and order <lo>/<hi>/<order>\n");
	GMT_message (GMT, "\t      where filter amplitudes = 0.5.  Replace wavelength by - to skip, e.g.,\n");
	GMT_message (GMT, "\t      try -F300/-/2 for a high-pass 2nd-order Butterworth filter.\n");
	GMT_message (GMT, "\t-I Integrate, i.e., divide by kr [ * scale].  Use -Ig to get m from mGal].\n");
	GMT_message (GMT, "\t-L Leave trend alone:  Do not remove least squares plane from data [Default removes plane].\n");
	GMT_message (GMT, "\t   Append m to just remove mean or h to remove mid-value instead.\n");
	GMT_message (GMT, "\t-N Choose or inquire about suitable grid dimensions for FFT, and set modifiers:\n");
	GMT_message (GMT, "\t   -Nf will force the FFT to use the dimensions of the data.\n");
	GMT_message (GMT, "\t   -Nq will inQuire about more suitable dimensions, report, then exit.\n");
	GMT_message (GMT, "\t   -N<nx>/<ny> will do FFT on array size <nx>/<ny> (Must be >= grid size).\n");
	GMT_message (GMT, "\t   Default chooses dimensions >= data which optimize speed, accuracy of FFT.\n");
	GMT_message (GMT, "\t   If FFT dimensions > grid dimensions, data are extended via edge point symmetry\n");
	GMT_message (GMT, "\t   and tapered to zero.  Several modifers can be set to change this behavior:\n");
	GMT_message (GMT, "\t     +e: Extend data via edge point symmetry [Default].\n");
	GMT_message (GMT, "\t     +m: Extend data via edge mirror symmetry.\n");
	GMT_message (GMT, "\t     +n: Do NOT extend data.\n");
	GMT_message (GMT, "\t     +t<w>: Limit tapering to <w> %% of the extended margin width and height [100].\n");
	GMT_message (GMT, "\t       If +n is set then +t instead sets the boundary width of the interior\n");
	GMT_message (GMT, "\t       grid margin to be tapered [0].\n");
	GMT_message (GMT, "\t-Q Save intermediate grid passed to FFT after detrending/extention/tapering.\n");
	GMT_message (GMT, "\t   Append output file prefix [tapered].  File name will be <prefix>_<ingrid>.\n");
	GMT_message (GMT, "\t-S multiply field by scale after inverse FFT [1.0].\n");
	GMT_message (GMT, "\t   Give -Sd to convert deflection of vertical to micro-radians.\n");
	GMT_message (GMT, "\t-T Compute isostatic response.  Input file is topo load. Append elastic thickness,\n");
	GMT_message (GMT, "\t   and densities of load, mantle, water, and infill, all in SI units.\n");
	GMT_message (GMT, "\t   It also implicitly sets -L.\n");
	GMT_message (GMT, "\t-Z Store raw complex spectrum to files real_<ingrid> and imag_<ingrid>.\n");
	GMT_message (GMT, "\t   Append p to store polar forms instead, i.e., mag_<ingrid> and phase_<ingrid>\n");
	GMT_explain_options (GMT, "Vf.");
	GMT_message (GMT, "\t-ho Write header record for spectral estimates (requires -E) [no header].\n");
	GMT_message (GMT, "\tList operations in the order desired for execution.\n");

	return (EXIT_FAILURE);
}

void add_operation_new (struct GMT_CTRL *C, struct GRDFFT_CTRL *Ctrl, int operation, unsigned int n_par, double *par)
{
	Ctrl->n_op_count++;
	Ctrl->operation = GMT_memory (C, Ctrl->operation, Ctrl->n_op_count, int);
	Ctrl->operation[Ctrl->n_op_count-1] = operation;
	if (n_par) {
		Ctrl->par = GMT_memory (C, Ctrl->par, Ctrl->n_par + n_par, double);
		GMT_memcpy (&Ctrl->par[Ctrl->n_par], par, n_par, double);
		Ctrl->n_par += n_par;
	}
}

int GMT_grdfft_new_parse (struct GMTAPI_CTRL *C, struct GRDFFT_CTRL *Ctrl, struct F_INFO *f_info, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdfft_new and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int j, k, n_errors = 0, filter_type = 0, pos = 0;
	int n_scan;
	char p[GMT_BUFSIZ], *c = NULL;
	double par[5];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	GMT_memset (f_info, 1, struct F_INFO);
	for (j = 0; j < 3; j++) {
		f_info->lc[j] = f_info->lp[j] = -1.0;		/* Set negative, below valid frequency range  */
		f_info->hp[j] = f_info->hc[j] = DBL_MAX;	/* Set huge positive, above valid frequency range  */
	}

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */
		GMT_memset (par, 5, double);

		switch (opt->option) {
			case '<':	/* Input file (only 1 or 2 are accepted) */
				Ctrl->In.active = true;
				if (Ctrl->In.n_grids < 2) 
					Ctrl->In.file[Ctrl->In.n_grids++] = strdup (opt->arg);
				else {
					n_errors++;
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error: A maximum of two input grids may be processed\n");
				}
				break;

			/* Processes program-specific parameters */

			case 'A':	/* Directional derivative */
				Ctrl->A.active = true;
				n_errors += GMT_check_condition (GMT, sscanf(opt->arg, "%lf", &par[0]) != 1, 
						"Syntax error -A option: Cannot read azimuth\n");
				add_operation_new (GMT, Ctrl, AZIMUTHAL_DERIVATIVE, 1, par);
				break;
			case 'C':	/* Upward/downward continuation */
				Ctrl->C.active = true;
				n_errors += GMT_check_condition (GMT, sscanf(opt->arg, "%lf", &par[0]) != 1, 
						"Syntax error -C option: Cannot read zlevel\n");
				add_operation_new (GMT, Ctrl, UP_DOWN_CONTINUE, 1, par);
				break;
			case 'D':	/* d/dz */
				Ctrl->D.active = true;
				par[0] = (opt->arg[0]) ? ((opt->arg[0] == 'g' || opt->arg[0] == 'G') ? MGAL_AT_45 : atof (opt->arg)) : 1.0;
				n_errors += GMT_check_condition (GMT, par[0] == 0.0, "Syntax error -D option: scale must be nonzero\n");
				add_operation_new (GMT, Ctrl, DIFFERENTIATE, 1, par);
				break;
			case 'E':	/* x,y,or radial spectrum, w for wavelength; k for km if geographical */ 
				Ctrl->E.active = true;
				j = 0;
				while (opt->arg[j]) {
					switch (opt->arg[j]) {
						case 'r': Ctrl->E.mode =  0; break;
						case 'x': Ctrl->E.mode = +1; break;
						case 'y': Ctrl->E.mode = -1; break;
						case 'w': Ctrl->E.give_wavelength = true; break;
						case 'k': if (Ctrl->E.give_wavelength) Ctrl->E.km = true; break;
					}
					j++;
				}
				par[0] = Ctrl->E.mode;
				add_operation_new (GMT, Ctrl, SPECTRUM, 1, par);
				break;
			case 'F':	/* Filter */
				Ctrl->F.active = true;
				if (!(f_info->set_already)) {
					filter_type = count_slashes_new (opt->arg);
					f_info->kind = FILTER_EXP + (filter_type - 1);
					f_info->set_already = true;
					add_operation_new (GMT, Ctrl, f_info->kind, 0, NULL);
				}
				n_errors += GMT_check_condition (GMT, parse_f_string_new (GMT, f_info, opt->arg), "Syntax error -F option");
				break;
			case 'G':	/* Output file */
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;
			case 'I':	/* Integrate */
				Ctrl->I.active = true;
				par[0] = (opt->arg[0] == 'g' || opt->arg[0] == 'G') ? MGAL_AT_45 : atof (opt->arg);
				n_errors += GMT_check_condition (GMT, par[0] == 0.0, "Syntax error -I option: scale must be nonzero\n");
				add_operation_new (GMT, Ctrl, INTEGRATE, 1, par);
				break;
			case 'L':	/* Leave trend alone */
				if (opt->arg[0] == 'm') Ctrl->L.mode = 1;
				else if (opt->arg[0] == 'h') Ctrl->L.mode = 2;
				else Ctrl->L.active = true;
				break;
#ifdef GMT_COMPAT
			case 'M':	/* Geographic data */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -M is deprecated; -fg was set instead, use this in the future.\n");
				if (!GMT_is_geographic (GMT, GMT_IN)) GMT_parse_common_options (GMT, "f", 'f', "g"); /* Set -fg unless already set */
				break;
#endif
			case 'N':	/* Grid dimension setting or inquiery */
				Ctrl->N.active = true;
				if ((c = strchr (opt->arg, '+'))) {	/* Handle modifiers */
					while ((GMT_strtok (c, "+", &pos, p))) {
						switch (p[0]) {
							case 'e':  Ctrl->N.info.taper_mode = GMT_FFT_EXTEND_POINT_SYMMETRY; break;
							case 'n':  Ctrl->N.info.taper_mode = GMT_FFT_EXTEND_NONE; break;
							case 'm':  Ctrl->N.info.taper_mode = GMT_FFT_EXTEND_MIRROR_SYMMETRY; break;
							case 't':  Ctrl->N.info.taper_width = atof (&p[1]); break;
							default: 
								GMT_report (GMT, GMT_MSG_NORMAL, "Error -N: Unrecognized modifier +%s.\n", p);
								n_errors++;
								break;
						}
					}
				}
				switch (opt->arg[0]) {
					case 'f': case 'F':
						Ctrl->N.info.info_mode = GMT_FFT_FORCE; break;
					case 'q': case 'Q':
						Ctrl->N.info.info_mode = GMT_FFT_QUERY; break;
					default:
						sscanf (opt->arg, "%d/%d", &Ctrl->N.info.nx, &Ctrl->N.info.ny);
						Ctrl->N.info.info_mode = GMT_FFT_SET;
				}
				break;
			case 'S':	/* Scale */
				Ctrl->S.active = true;
				Ctrl->S.scale = (opt->arg[0] == 'd' || opt->arg[0] == 'D') ? 1.0e6 : atof (opt->arg);
				break;
#ifdef GMT_COMPAT
			case 'T':	/* Flexural isostasy */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -T is deprecated; see gravfft for isostasy and gravity calcuations.\n");
				Ctrl->T.active = Ctrl->L.active = true;
				n_scan = sscanf (opt->arg, "%lf/%lf/%lf/%lf/%lf", &par[0], &par[1], &par[2], &par[3], &par[4]);
				for (j = 1, k = 0; j < 5; j++) if (par[j] < 0.0) k++;
				n_errors += GMT_check_condition (GMT, n_scan != 5 || k > 0, 
					"Syntax error -T option: Correct syntax:\n\t-T<te>/<rhol>/<rhom>/<rhow>/<rhoi>, all densities >= 0\n");
				add_operation_new (GMT, Ctrl, ISOSTASY, 5, par);
				break;
#endif
#ifdef DEBUG
			case '=':	/* Do nothing */
				add_operation_new (GMT, Ctrl, -1, 1, par);
				if (opt->arg[0] == '+') show_n_new = true;
				break;
#endif
			case 'Q':	/* Output intermediate grid file */
				Ctrl->Q.active = true;
				if (opt->arg[0]) { free (Ctrl->Q.prefix); Ctrl->Q.prefix = strdup (opt->arg);}
				break;
			case 'Z':	/* Output raw complex spectrum */
				Ctrl->Z.active = true;
				if (opt->arg[0] == 'p') Ctrl->Z.mode = 1;	/* Store mag,phase instead of real,imag */
				break;
					break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, !(Ctrl->n_op_count), "Syntax error: Must specify at least one operation\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.info.info_mode == GMT_FFT_SET && (Ctrl->N.info.nx <= 0 || Ctrl->N.info.ny <= 0), 
			"Syntax error -N option: nx2 and/or ny2 <= 0\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.scale == 0.0, "Syntax error -S option: scale must be nonzero\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.info.taper_mode == GMT_FFT_EXTEND_NONE && Ctrl->N.info.taper_width == 100.0, "Syntax error -N option: +n requires +t with width << 100!\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->In.file, "Syntax error: Must specify input file\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->E.active && !Ctrl->G.file, "Syntax error -G option: Must specify output file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdfft_new_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdfft_new (void *V_API, int mode, void *args)
{
	bool error = false, stop;
	int status;
	unsigned int op_count = 0, par_count = 0, k;
	uint64_t ij;

	struct GMT_GRID *Grid[2] = {NULL,  NULL}, *Orig[2] = {NULL,  NULL};
	struct F_INFO f_info;
	struct GMT_FFT_WAVENUMBER *FFT_info[2] = {NULL, NULL}, *K = NULL;
	struct GRDFFT_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdfft_new_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdfft_new_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-Vfh", "", options)) Return (API->error);
	Ctrl = New_grdfft_new_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdfft_new_parse (API, Ctrl, &f_info, options))) Return (error);

	/*---------------------------- This is the grdfft_new main code ----------------------------*/

	for (k = 0; k < Ctrl->In.n_grids; k++) {	/* Get the grid header(s) */
		if ((Orig[k] = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->In.file[k], NULL)) == NULL)	/* Get header only */
			Return (API->error);
	}

	if (Ctrl->In.n_grids == 2) {	/* Make sure grids are co-registered and same size, registration, etc. */
		if(Orig[0]->header->registration != Orig[1]->header->registration) {
			GMT_report (GMT, GMT_MSG_NORMAL, "The two grids have different registrations!\n");
			Return (EXIT_FAILURE);
		}
		if (!GMT_grd_same_shape (GMT, Orig[0], Orig[1])) {
			GMT_report (GMT, GMT_MSG_NORMAL, "The two grids have different dimensions\n");
			Return (EXIT_FAILURE);
		}
		if (!GMT_grd_same_region (GMT, Orig[0], Orig[1])) {
			GMT_report (GMT, GMT_MSG_NORMAL, "The two grids have different regions\n");
			Return (EXIT_FAILURE);
		}
		if (!GMT_grd_same_inc (GMT, Orig[0], Orig[1])) {
			GMT_report (GMT, GMT_MSG_NORMAL, "The two grids have different intervals\n");
			Return (EXIT_FAILURE);
		}
	}

	for (k = 0; k < Ctrl->In.n_grids; k++) {	/* Read, and check that no NaNs are present in either grid */
		GMT_grd_init (GMT, Orig[k]->header, options, true);	/* Update the header */
		FFT_info[k] = GMT_grd_fft_init (GMT, Orig[k], &Ctrl->N.info);
		
		if ((Orig[k] = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA | GMT_GRID_COMPLEX_REAL, NULL, Ctrl->In.file[k], Orig[k])) == NULL)	/* Get data only */
			Return (API->error);
		for (ij = 0, stop = false; !stop && ij < Orig[k]->header->size; ij++) stop = GMT_is_fnan (Orig[k]->data[ij]);
		if (stop) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Input grid %s contain NaNs, aborting!\n", Ctrl->In.file[k]);
			Return (EXIT_FAILURE);
		}
	}
	
	K = FFT_info[0];	/* We only need one of these anyway */
	
	/* Note: If input grid(s) are read-only then we must duplicate; otherwise Grid[k] points to Orig[k] */
	for (k = 0; k < Ctrl->In.n_grids; k++) {
		(void) GMT_set_outgrid (GMT, Orig[k], &Grid[k]);
	}
	
	/* From here we address the first grid via Grid[0] and the 2nd grid as Grid[1];
	 * we are done with the addresses Orig[k] although they may be the same pointers. */
	
	for (k = 0; k < Ctrl->In.n_grids; k++) {	/* Detrend (if requested), extend/taper (if requested) */
		if (!(Ctrl->L.active)) GMT_grd_detrend (GMT, Grid[k], Ctrl->L.mode);
		GMT_grd_taper_edges (GMT, Grid[k], &Ctrl->N.info);
		if (Ctrl->Q.active) GMT_grd_save_taper (GMT, Grid[k], Ctrl->Q.prefix);
	}

#ifdef FTEST
	/* PW: Used with -DFTEST to check that the radial filters compute correctly */
	{
		double f = 0.0;
		while (f < 3.0) {
			printf ("%g\t%g\n", f, f_info.filter (f, 0));	/* Radial filter */
			f += 0.01;
		}
		exit (-1);
	}
#endif

	for (k = 0; k < Ctrl->In.n_grids; k++) {	/* Call the forward FFT */
		GMT_report (GMT, GMT_MSG_VERBOSE, "forward FFT...\n");
		if (GMT_fft_2d (GMT, Grid[k]->data, Ctrl->N.info.nx, Ctrl->N.info.ny, k_fft_fwd, k_fft_complex))
			Return (EXIT_FAILURE);
		if (Ctrl->Z.active) GMT_grd_save_fft (GMT, Grid[k], Ctrl->Z.mode, K, Ctrl->In.file[k]);
	}

#if 1
	for (op_count = par_count = 0; op_count < Ctrl->n_op_count; op_count++) {
		switch (Ctrl->operation[op_count]) {
			case UP_DOWN_CONTINUE:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) ((Ctrl->par[par_count] < 0.0) ? GMT_message (GMT, "downward continuation...\n") : GMT_message (GMT,  "upward continuation...\n"));
				par_count += do_continuation_new (Grid[0], &Ctrl->par[par_count], K);
				break;
			case AZIMUTHAL_DERIVATIVE:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "azimuthal derivative...\n");
				par_count += do_azimuthal_derivative_new (Grid[0], &Ctrl->par[par_count], K);
				break;
			case DIFFERENTIATE:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "differentiate...\n");
				par_count += do_differentiate_new (Grid[0], &Ctrl->par[par_count], K);
				break;
			case INTEGRATE:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "integrate...\n");
				par_count += do_integrate_new (Grid[0], &Ctrl->par[par_count], K);
				break;
#ifdef GMT_COMPAT
			case ISOSTASY:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "isostasy...\n");
				par_count += do_isostasy_new (Grid[0], Ctrl, &Ctrl->par[par_count], K);
				break;
#endif
			case FILTER_COS:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "cosine filter...\n");
				do_filter_new (Grid[0], &f_info, K);
				break;
			case FILTER_EXP:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "Gaussian filter...\n");
				do_filter_new (Grid[0], &f_info, K);
				break;
			case FILTER_BW:
				if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "Butterworth filter...\n");
				do_filter_new (Grid[0], &f_info, K);
				break;
			case SPECTRUM:	/* This operator writes a table to file (or stdout if -G is not used) */
				if (Ctrl->In.n_grids == 2) {	/* Cross-spectral calculations */
					if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "cross-spectra...\n");
				}
				else {	/* Spectral calculations */
					if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "spectrum...\n");
				}
				status = do_spectrum_new (GMT, Grid[0], Grid[1], &Ctrl->par[par_count], Ctrl->E.give_wavelength, Ctrl->E.km, Ctrl->G.file, K);
				if (status < 0) Return (status);
				par_count += status;
				break;
		}
	}
#endif
	if (!Ctrl->E.active) {	/* Since -E out was handled separately by do_spectrum_new */
		if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "inverse FFT...\n");

		if (GMT_fft_2d (GMT, Grid[0]->data, Ctrl->N.info.nx, Ctrl->N.info.ny, k_fft_inv, k_fft_complex))
			Return (EXIT_FAILURE);

		/* FFT computes an unnormalized transform, in that there is no
		 * coefficient in front of the summation in the FT. In other words,
		 * applying the forward and then the backward transform will multiply the
		 * input by the number of elements (header->size). Here we correct this: */
		Ctrl->S.scale *= (2.0 / Grid[0]->header->nm);
		GMT_scale_and_offset_f (GMT, Grid[0]->data, Grid[0]->header->size, Ctrl->S.scale, 0);

		/* The data are in the middle of the padded array */
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA | GMT_GRID_COMPLEX_REAL, NULL, Ctrl->G.file, Grid[0]) != GMT_OK) {
			Return (API->error);
		}
	}

	for (k = 0; k < Ctrl->In.n_grids; k++) GMT_free (GMT, FFT_info[k]);

	Return (EXIT_SUCCESS);
}
