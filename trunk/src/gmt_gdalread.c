/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU Lesser General Public License as published by
 *      the Free Software Foundation; version 3 or any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU Lesser General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/* Program:	GMT_gdalread.c
 * Purpose:	routine to read files supported by gdal
 * 		and dumping all or selected band data of that dataset.
 *
 * Author:	Joaquim Luis
 * Date:	27-Aug-2009
 * Revision: 1		Based on gdalread.c MEX from Mirone
 *			The populate_metadata() was modified from mexgdal of John Evans (johnevans@acm.org)
 *
 */

int record_geotransform (char *gdal_filename, GDALDatasetH hDataset, double *adfGeoTransform);
int populate_metadata (struct GMT_CTRL *C, struct GD_CTRL *Ctrl, char *gdal_filename, int got_R, int nXSize, int nYSize, double dfULX, double dfULY, double dfLRX, double dfLRY, double z_min, double z_max);
int ReportCorner (struct GMT_CTRL *C, GDALDatasetH hDataset, double x, double y, double *xy_c, double *xy_geo);
void ComputeRasterMinMax (struct GMT_CTRL *C, unsigned char *tmp, GDALRasterBandH hBand, double adfMinMax[2], int nXSize, int nYSize, double, double);
int gdal_decode_columns (struct GMT_CTRL *C, char *txt, int *whichBands, unsigned int n_col);

int GMT_is_gdal_grid (struct GMT_CTRL *C, struct GRD_HEADER *header) {
	GDALDatasetH hDataset;

	GDALAllRegister();
	hDataset = GDALOpen(header->name, GA_ReadOnly);

	if (hDataset == NULL) return (GMT_GRDIO_BAD_VAL);
	GMT_report (C, GMT_MSG_VERBOSE, "File %s reads with GDAL driver %s\n", header->name, GDALGetDriverShortName(GDALGetDatasetDriver(hDataset)));
	GDALClose (hDataset);
	header->type = GMT_GRD_IS_GD;

	return (header->type);
}

int GMT_gdalread (struct GMT_CTRL *C, char *gdal_filename, struct GDALREAD_CTRL *prhs, struct GD_CTRL *Ctrl) {
	const char	*format = NULL;
	int	nRGBA = 1;	/* 1 for BSQ; 3 for RGB and 4 for RGBA (If needed, value is updated bellow) */
	int	complex_mode = 0;	/* 0 real only. 1|2 if complex array is to hold real (1) and imaginary (2) parts */
	int	nPixelSize, nBands, i, nReqBands = 0;
	int	anSrcWin[4], xOrigin = 0, yOrigin = 0;
	int	jump = 0, nXSize = 0, nYSize = 0, nX, nY, nBufXSize, nBufYSize;
	int	n, m, incStep = 1;
	bool	do_BIP;		/* For images if BIP == true data is stored Pixel interleaved, otherwise Band interleaved */
	bool	metadata_only;
	bool	pixel_reg = false;	/* GDAL decides everything is pixel reg, we make our decisions based on data type */
	bool	fliplr, got_R = false, got_r = false, error = false;
	int	*whichBands = NULL, *mVector = NULL, *nVector = NULL;
	int	off, pad = 0, i_x_nXYSize, startColPos, nXSize_withPad;
	unsigned int nn;
	size_t n_alloc;
	//int	incStep = 1;	/* 1 for real only arrays and 2 for complex arrays (index step increment) */
	unsigned char *tmp = NULL;
	double	tmpF64, adfMinMax[2];
	double	dfULX = 0.0, dfULY = 0.0, dfLRX = 0.0, dfLRY = 0.0;
	double	z_min = 1e50, z_max = -1e50;
	GDALDatasetH	hDataset;
	GDALRasterBandH	hBand;
	GDALDriverH	hDriver;

	Ctrl->band_field_names = NULL;		/* So we can test before trying to read its fields */
	GMT_memset (anSrcWin, 4, int);

	if (prhs->B.active) {		/* We have a selected bands request */
		int nc_ind, n_commas = 0, n_dash = 0;
		for (nc_ind = 0; prhs->B.bands[nc_ind]; nc_ind++)
			if (prhs->B.bands[nc_ind] == ',') n_commas++;
		for (n = 0; prhs->B.bands[n]; n++)
			if (prhs->B.bands[n] == '-') n_dash = (int)n;
		nn = MAX(n_commas+1, n_dash);
		if (nn) {
			nn = MAX( nn, (unsigned int)atoi(&prhs->B.bands[nc_ind-1])+1 );		/* +1 because band numbering in GMT is zero based */
			if (n_dash)	nn = MAX( nn, (unsigned int)atoi(&prhs->B.bands[nn+1])+1 );
		}
		else		/* Hmm, this else case is never reached */
			nn = atoi(prhs->B.bands);
		whichBands = GMT_memory (C, NULL, nn, int);
		nReqBands = gdal_decode_columns (C, prhs->B.bands, whichBands, nn);
		free(prhs->B.bands);	/* This is actualy the contents of header->pocket allocated by strdup */
		prhs->B.bands = NULL;
	}
	else if (prhs->f_ptr.active) {
		/* Here we are going to read to a grid so if no band info was provided, default to read only the
		   first band. This avoids, for example, allocate and read all 3 bands in a RGB image and send
		   back a full 3 band array to GMT_gdal_read_grd that will only keep the first band. */
		nReqBands = 1;
		whichBands = GMT_memory (C, NULL, 1, int);
		whichBands[0] = 1;
	}

	do_BIP = prhs->I.active;
	if (nReqBands == 1) do_BIP = false;	/* This must overrule any -I option settings */
	fliplr = prhs->L.active;
	metadata_only = prhs->M.active;

	if (prhs->p.active) pad = prhs->p.pad;

	if (prhs->R.active) {
		got_R = true;
		C->common.R.active = false;	/* Reset because -R was already parsed when reading header info */
		error += GMT_parse_common_options (C, "R", 'R', prhs->R.region);
		if (!error) {
			double dx = 0, dy = 0;
			if (!prhs->registration.val) {	/* Subregion coords are grid-reg. Need to convert to pix-reg */
				dx = prhs->registration.x_inc / 2;
				dy = prhs->registration.y_inc / 2;
			}
			dfULX = C->common.R.wesn[XLO] - dx;
			dfLRX = C->common.R.wesn[XHI] + dx;
			dfLRY = C->common.R.wesn[YLO] - dy;
			dfULY = C->common.R.wesn[YHI] + dy;
		}
	}

	if (prhs->r.active) { 		/* Region is given in pixels */
		got_r = true;
		C->common.R.active = false;
		error += GMT_parse_common_options (C, "R", 'R', prhs->r.region);
		if (!error) {
			dfULX = C->common.R.wesn[XLO];	dfLRX = C->common.R.wesn[XHI];
			dfLRY = C->common.R.wesn[YLO];	dfULY = C->common.R.wesn[YHI];
		}
	}

	if (error) {
		GMT_report (C, GMT_MSG_NORMAL, "Error: GMT_gdalread failed to extract a Sub-region\n");
		return (-1);
	}

	if (prhs->P.active)
		jump = atoi(prhs->P.jump);

	if (prhs->Z.active) {
		complex_mode = prhs->Z.complex_mode;
		if (complex_mode) incStep = 2;
	}

	/* Open gdal - */

	GDALAllRegister();

	if (prhs->W.active) {
		OGRSpatialReferenceH  hSRS;
		/* const char *str = Ctrl->ProjectionRefPROJ4; */

		hSRS = OSRNewSpatialReference(NULL);

		if( OSRImportFromProj4( hSRS, Ctrl->ProjectionRefPROJ4) == CE_None ) {
			char	*pszPrettyWkt = NULL;
			OSRExportToPrettyWkt( hSRS, &pszPrettyWkt, false );
			Ctrl->ProjectionRefWKT = pszPrettyWkt;
		}
		else {
			Ctrl->ProjectionRefWKT = NULL;
			GMT_report (C, GMT_MSG_NORMAL, "Warning: GMT_gdalread failed to convert the proj4 string\n%s\n to WKT\n", 
					Ctrl->ProjectionRefPROJ4);
		}

		OSRDestroySpatialReference( hSRS );
		return (GMT_NOERROR);
	}

	if (metadata_only) {
		populate_metadata (C, Ctrl, gdal_filename, got_R, nXSize, nYSize, dfULX, dfULY, dfLRX, dfLRY, z_min, z_max);

		/* Return registration based on data type of first band. Byte is pixel reg otherwise set grid registration */
		if (!Ctrl->hdr[6]) {		/* Grid registration */
			Ctrl->hdr[0] += Ctrl->hdr[7] / 2;	Ctrl->hdr[1] -= Ctrl->hdr[7] / 2;
			Ctrl->hdr[2] += Ctrl->hdr[8] / 2;	Ctrl->hdr[3] -= Ctrl->hdr[8] / 2;
		}
		return (GMT_NOERROR);
	}

	hDataset = GDALOpen(gdal_filename, GA_ReadOnly);

	if (hDataset == NULL) {
		GMT_report (C, GMT_MSG_NORMAL, "GDALOpen failed %s\n", CPLGetLastErrorMsg());
		return (-1);
	}

	/* Some formats (tipically DEMs) have their origin at Bottom Left corner.
	   For those we have to flip the data matrix to be in accord with matlab (Top Left) */

	hDriver = GDALGetDatasetDriver(hDataset);
	format = GDALGetDriverShortName(hDriver);

	if (!strcmp(format,"ESAT"))	/* ENVISAT data are flipped left-right */
		fliplr = true;

	if (got_R || got_r) {
		/* -------------------------------------------------------------------- */
		/*      Compute the source window from the projected source window      */
		/*      if the projected coordinates were provided.  Note that the      */
		/*      projected coordinates are in ulx, uly, lrx, lry format,         */
		/*      while the anSrcWin is xoff, yoff, xsize, ysize with the         */
		/*      xoff,yoff being the ulx, uly in pixel/line.                     */
		/* -------------------------------------------------------------------- */
		double	adfGeoTransform[6];

		GDALGetGeoTransform( hDataset, adfGeoTransform );

		if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 ) {
			GMT_report (C, GMT_MSG_NORMAL, "The -projwin option was used, but the geotransform is rotated. This configuration is not supported.\n");
			GDALClose( hDataset );
			GDALDestroyDriverManager();
			return (-1);
		}

		if (got_R) {	/* Region in map coordinates */
			anSrcWin[0] = (int) ((dfULX - adfGeoTransform[0]) / adfGeoTransform[1] + 0.001);
			anSrcWin[1] = (int) ((dfULY - adfGeoTransform[3]) / adfGeoTransform[5] + 0.001);
			anSrcWin[2] = (int) ((dfLRX - dfULX) / adfGeoTransform[1] + 0.5);
			anSrcWin[3] = (int) ((dfLRY - dfULY) / adfGeoTransform[5] + 0.5);
			if (GDAL_VERSION_NUM < 1700 && !strcmp(format,"netCDF")) {
				/* PATCH against the old GDAL bugs of reading netCDF files */
				anSrcWin[1] = GDALGetRasterYSize(hDataset) - (anSrcWin[1] + anSrcWin[3]) - 1;
			}
		}
		else {		/* Region in pixel/line */
			anSrcWin[0] = (int) (dfULX);
			anSrcWin[1] = (int) (dfLRY);
			anSrcWin[2] = (int) (dfLRX - dfULX);
			anSrcWin[3] = (int) (dfULY - dfLRY);
		}

		if( anSrcWin[0] < 0 || anSrcWin[1] < 0
			|| anSrcWin[0] + anSrcWin[2] > GDALGetRasterXSize(hDataset)
			|| anSrcWin[1] + anSrcWin[3] > GDALGetRasterYSize(hDataset) ) {
			GMT_report (C, GMT_MSG_NORMAL, "Computed -srcwin falls outside raster size of %dx%d.\n",
					GDALGetRasterXSize(hDataset), GDALGetRasterYSize(hDataset));
			return (-1);
		}
		xOrigin = anSrcWin[0];
		yOrigin = anSrcWin[1];
		nXSize = anSrcWin[2];
		nYSize = anSrcWin[3];
	}
	else {			/* Use entire dataset */
		xOrigin = yOrigin = 0;
		nXSize = GDALGetRasterXSize(hDataset);
		nYSize = GDALGetRasterYSize(hDataset);
	}

	/* The following assumes that all bands have the same PixelSize. Otherwise ... */
	hBand = GDALGetRasterBand(hDataset,1);
	nPixelSize = GDALGetDataTypeSize(GDALGetRasterDataType(hBand)) / 8;	/* /8 because return value is in BITS */

	if (jump) {
		nBufXSize = GDALGetRasterXSize(hDataset) / jump;
		nBufYSize = GDALGetRasterYSize(hDataset) / jump;
	}
	else {
		nBufXSize = nXSize;	nBufYSize = nYSize;
	}

	nBands = GDALGetRasterCount(hDataset);

	if (nReqBands) nBands = MIN(nBands,nReqBands);	/* If a band selection was made */

	n_alloc = nBands * (nBufXSize + 2*pad) * (nBufYSize + 2*pad);
	switch ( GDALGetRasterDataType(hBand) ) {
		case GDT_Byte:
			if (prhs->c_ptr.active)	/* We have a pointer with already allocated memory ready to use */
				Ctrl->UInt8.data = prhs->c_ptr.grd;
			else
				Ctrl->UInt8.data = GMT_memory (C, NULL, n_alloc, uint8_t); /* aka unsigned char */

			if (do_BIP) {
				if (nBands == 4)	/* Assume fourth band holds the alpha channel */
					nRGBA = 4;
				else if (nBands == 3)
					nRGBA = 3;
				else {
					/* BIP request ignored since number of bands is not 3 or 4 */
					do_BIP = false;
				}
			}
			break;
		case GDT_Int16:
			Ctrl->Int16.data = GMT_memory (C, NULL, n_alloc, int16_t);
			break;
		case GDT_UInt16:
			Ctrl->UInt16.data = GMT_memory (C, NULL, n_alloc, uint16_t);
			break;
		case GDT_Int32:
			Ctrl->Int32.data = GMT_memory (C, NULL, n_alloc, int32_t);
			break;
		case GDT_UInt32:
			Ctrl->UInt32.data = GMT_memory (C, NULL, n_alloc, uint32_t);
			break;
		case GDT_Float32:
		case GDT_Float64:
			if (prhs->f_ptr.active)	/* We have a pointer with already allocated memory ready to use */
				Ctrl->Float.data = prhs->f_ptr.grd;
			else {
				if (!complex_mode)
					Ctrl->Float.data = GMT_memory (C, NULL, n_alloc, float);
				else
					Ctrl->Float.data = GMT_memory (C, NULL, 2 * n_alloc, float);
			}
			break;
		default:
			GMT_report (C, GMT_MSG_NORMAL, "gdalread: Unsupported data type\n");
			break;
	}

	tmp = calloc(nBufYSize * nBufXSize, nPixelSize);
	if (tmp == NULL) {
		GMT_report (C, GMT_MSG_NORMAL, "gdalread: failure to allocate enough memory\n");
		return(-1);
	}

	/* ------ compute two vectors indices that will be used inside loops below --------- */
	/* In the "Preview" mode those guys bellow are different and what we need is the BufSize */
	if (jump)
		nX = nBufXSize,	nY = nBufYSize;
	else
		nX = nXSize,	nY = nYSize;

	mVector = GMT_memory(C, NULL, nY, int);
	for (m = 0; m < nY; m++) mVector[m] = m*nX;
	nVector = GMT_memory(C, NULL, nX+4*pad, int);	/* For now this will be used only to select BIP ordering */
	/* --------------------------------------------------------------------------------- */

	for ( i = 0; i < nBands; i++ ) {
		if (!nReqBands)		/* No band selection, read them sequentialy */
			hBand = GDALGetRasterBand( hDataset, i+1 );
		else			/* Band selection. Read only the requested ones */
			hBand = GDALGetRasterBand( hDataset, (int)whichBands[i] );

		/* Decide if grid or pixel registration based on the data type of first band actually sent back to the GMT machinery */
		if (i == 0) pixel_reg = (GDALGetRasterDataType(hBand) == GDT_Byte);

		GDALRasterIO(hBand, GF_Read, xOrigin, yOrigin, nXSize, nYSize,
			tmp, nBufXSize, nBufYSize, GDALGetRasterDataType(hBand), 0, 0 );

		/* If we didn't computed it yet, its time to do it now */
		if (got_R) ComputeRasterMinMax(C, tmp, hBand, adfMinMax, nXSize, nYSize, z_min, z_max);

		/* In the "Preview" mode those guys bellow are different and what we need is the BufSize */
		if (jump) {
			nXSize = nBufXSize;
			nYSize = nBufYSize;
			i_x_nXYSize = i*(nXSize+2*pad)*(nYSize+2*pad);		/* We don't need to recompute this everytime */
		}
		else
			i_x_nXYSize= i * (nBufXSize + 2*pad) * (nBufYSize + 2*pad);

		nXSize_withPad = nXSize + 2 * pad;
		startColPos = pad + i_x_nXYSize + (complex_mode > 1);	/* Take into account nBands, Padding and Complex */

		switch ( GDALGetRasterDataType(hBand) ) {
			case GDT_Byte:
				/* This chunk is kind of complicated because we want to take into account several different cases */
				for (n = 0; n < nXSize; n++)
					if (do_BIP)
						nVector[n] = n * nRGBA + i;	/* Vector for Pixel Interleaving */
					else
						nVector[n] = n + i_x_nXYSize;	/* Vector for Band Sequential */

				Ctrl->UInt8.active = true;
				if (fliplr) {				/* No BIP option yet, and maybe never */
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = nXSize-1; n >= 0; n--)
							Ctrl->UInt8.data[nn++] = tmp[mVector[m]+n];
					}
				}
				else
					for (m = 0; m < nYSize; m++) {
						/*nn = pad + (pad+m)*(nXSize + 2*pad) + i_x_nXYSize;*/
						off = nRGBA * pad + (pad+m) * (nRGBA * (nXSize_withPad)); /* Remember, nRGBA is variable */
						for (n = 0; n < nXSize; n++)
							Ctrl->UInt8.data[nVector[n] + off] = tmp[mVector[m]+n];
							/*Ctrl->UInt8.data[nn++] = tmp[mVector[m]+n];*/
					}
				break;
			case GDT_Int16:
				Ctrl->Int16.active = true;
				if (fliplr) {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = nXSize-1; n >= 0; n--)
							memcpy (&Ctrl->Int16.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(int16_t)], sizeof(int16_t));
					}
				}
				else {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = 0; n < nXSize; n++)
							memcpy (&Ctrl->Int16.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(int16_t)], sizeof(int16_t));
					}
				}
				break;
			case GDT_UInt16:
				Ctrl->UInt16.active = true;
				if (fliplr) {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = nXSize-1; n >= 0; n--)
							memcpy (&Ctrl->UInt16.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(uint16_t)], sizeof(uint16_t));
					}
				}
				else {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = 0; n < nXSize; n++)
							memcpy (&Ctrl->UInt16.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(uint16_t)], sizeof(uint16_t));
					}
				}
				break;
			case GDT_Int32:
				Ctrl->Int32.active = true;
				if (fliplr) {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = nXSize-1; n >= 0; n--)
							memcpy (&Ctrl->Int32.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(int32_t)], sizeof(int32_t));
					}
				}
				else {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = 0; n < nXSize; n++)
							memcpy (&Ctrl->Int32.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(int32_t)], sizeof(int32_t));
					}
				}
				break;
			case GDT_UInt32:
				Ctrl->UInt32.active = true;
				if (fliplr) {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = nXSize-1; n >= 0; n--)
							memcpy (&Ctrl->UInt32.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(uint32_t)], sizeof(uint32_t));
					}
				}
				else {
					for (m = 0; m < nYSize; m++) {
						nn = pad + (pad+m)*(nXSize_withPad) + i_x_nXYSize;
						for (n = 0; n < nXSize; n++)
							memcpy (&Ctrl->UInt32.data[nn++],
									&tmp[(mVector[m]+n) * sizeof(uint32_t)], sizeof(uint32_t));
					}
				}
				break;
			case GDT_Float32:
				Ctrl->Float.active = true;
				for (m = 0; m < nYSize; m++) {
					nn = (pad+m)*(nXSize_withPad) + startColPos;
					for (n = 0; n < nXSize; n++) {
						memcpy (&Ctrl->Float.data[nn],
								&tmp[(mVector[m]+n) * sizeof(float)], sizeof(float));
						nn += incStep;
					}
				}
				break;
			case GDT_Float64:	/* For now we don't care about doubles */
				Ctrl->Float.active = true;
				for (m = 0; m < nYSize; m++) {
					nn = (pad+m)*(nXSize_withPad) + startColPos;
					for (n = 0; n < nXSize; n++) {
						memcpy (&tmpF64,
								&tmp[(mVector[m]+n) * sizeof(double)], sizeof(double));
						Ctrl->Float.data[nn] = (float)tmpF64;
						nn += incStep;
					}
				}
				break;
			default:
				CPLAssert( false );
		}
	}

	GMT_free(C, mVector);
	free(tmp);
	if (whichBands) GMT_free(C, whichBands);
	if (nVector) GMT_free(C, nVector);

	GDALClose(hDataset);

	populate_metadata (C, Ctrl, gdal_filename, got_R, nXSize, nYSize, dfULX, dfULY, dfLRX, dfLRY, z_min, z_max);

	/* Return registration based on data type of the actually read first band.
	   We do this at the end because 'populate_metadata' scans all bands in file 
	   and cannot know which one was actually read. */
	if (!pixel_reg) {		/* Grid registration */
		Ctrl->hdr[0] += Ctrl->hdr[7] / 2;	Ctrl->hdr[1] -= Ctrl->hdr[7] / 2;
		Ctrl->hdr[2] += Ctrl->hdr[8] / 2;	Ctrl->hdr[3] -= Ctrl->hdr[8] / 2;
	}

	Ctrl->nActualBands = nBands;	/* Number of bands that were actually read in */

	return (GMT_NOERROR);
}

/* =============================================================================================== */
/*
 * This routine queries the GDAL raster file for some metadata
 *
 * Fields:
 *    ProjectionRefPROJ4:  a Proj4 type string describing the projection
 *    GeoTransform:
 *        a 6-tuple.  Entries are as follows.
 *            [0] --> top left x
 *            [1] --> w-e pixel resolution
 *            [2] --> rotation, 0 if image is "north up"
 *            [3] --> top left y
 *            [4] --> rotation, 0 if image is "north up"
 *            [5] --> n-s pixel resolution
 *
 *    DriverShortName:  describes the driver used to query *this* raster file
 *    DriverLongName:   describes the driver used to query *this* raster file
 *    RasterXSize, RasterYSize:
 *        These are the primary dimensions of the raster.  See "Overview", though.
 *    RasterCount:
 *        Number of raster bands present in the file.
 *
 *    ColorMap:
 *        A Mx4 int array with the colormap of first band, or NULL if it does not exists
 *
 *    ColorInterp:
 *
 *    Corners:
 *        Also a structure array with the Fields:
 *            LL, UL, UR, LR. Each of this contains the (x,y) coords (or pixel/line coords
 *            if image is not referenced) of the LL - LowerLeft - point, and so on.
 *
 *    GEOGCorners:
 *        A structure array with the same field as 'Corners' but in geographical coordinates
 *        (if that is possible, or otherwise the LL values are set to NaN).
 *
 *    hdr:
 *        contains a row vector with (xmin, xmax, ymin, ymax, zmin, zmax, pixel_reg, xinc, yinc)
 *        for this data set. Pixel_reg is 1 for pixel registration and 0 for grid node
 *        registration, but it is actually set the -F option [default is 0]
 *
 * */
int populate_metadata (struct GMT_CTRL *C, struct GD_CTRL *Ctrl, char *gdal_filename, int got_R, int nXSize, int nYSize, double dfULX, double dfULY, double dfLRX, double dfLRY, double z_min, double z_max) {

	GDALDriverH hDriver;		/* This is the driver chosen by the GDAL library to query the dataset. */
	GDALDatasetH hDataset;		/* pointer structure used to query the gdal file. */
	GDALRasterBandH hBand;
	GDALColorTableH	hTable;
	GDALColorEntry	sEntry;

	int	i, j, n_colors;		/* Number of colors in the eventual Color Table */
	int	status, bSuccess;	/* success or failure */
	int	nBand, raster_count, pixel_reg = false;
	double 	adfGeoTransform[6];	/* bounds on the dataset */
	double	tmpdble;		/* temporary value */
	double	xy_c[2], xy_geo[4][2];	/* Corner coordinates in the local coords system and geogs (if it exists) */
	int	bGotMin, bGotMax;	/* To know if driver transmited Min/Max */
	double	adfMinMax[2];		/* Dataset Min Max */
	double	nodata;

	/* ------------------------------------------------------------------------- */
	/* Open the file (if we can). */
	/* ------------------------------------------------------------------------- */
	hDataset = GDALOpen ( gdal_filename, GA_ReadOnly );
	if ( hDataset == NULL ) {
		GMT_report (C, GMT_MSG_NORMAL, "Unable to open %s.\n", gdal_filename );
		return ( -1 );
	}

	/* ------------------------------------------------------------------------- */
	/* Record the ProjectionRef */
	/* ------------------------------------------------------------------------- */
	if( GDALGetProjectionRef( hDataset ) != NULL ) {
		OGRSpatialReferenceH  hSRS;
		char *pszProjection;
		char *pszResult = NULL;
		pszProjection = (char *)GDALGetProjectionRef( hDataset );

		hSRS = OSRNewSpatialReference(NULL);
		/* First in PROJ4 format */
		if( OSRImportFromWkt( hSRS, &pszProjection) == CE_None ) {
			OSRExportToProj4( hSRS, &pszResult );
			Ctrl->ProjectionRefPROJ4 = pszResult;
		}
		else
			Ctrl->ProjectionRefPROJ4 = NULL;

		/* Now in WKT format */
		if( OSRImportFromWkt( hSRS, &pszProjection ) == CE_None ) {
			char	*pszPrettyWkt = NULL;
			OSRExportToPrettyWkt( hSRS, &pszPrettyWkt, false );
			Ctrl->ProjectionRefWKT = pszPrettyWkt;
			CPLFree( pszPrettyWkt );
		}
		else
			Ctrl->ProjectionRefWKT = NULL;

		OSRDestroySpatialReference( hSRS );
	}

	/* ------------------------------------------------------------------------- */
	/* Record the geotransform. */
	/* ------------------------------------------------------------------------- */
	status = record_geotransform ( gdal_filename, hDataset, adfGeoTransform );
	if (!strcmp(GDALGetDriverShortName(GDALGetDatasetDriver(hDataset)),"netCDF") && GDAL_VERSION_NUM <= 1450) {
		adfGeoTransform[3] *= -1;
		adfGeoTransform[5] *= -1;
	}
	if (status == 0) {
		Ctrl->GeoTransform[0] = adfGeoTransform[0];
		Ctrl->GeoTransform[1] = adfGeoTransform[1];
		Ctrl->GeoTransform[2] = adfGeoTransform[2];
		Ctrl->GeoTransform[3] = adfGeoTransform[3];
		Ctrl->GeoTransform[4] = adfGeoTransform[4];
		Ctrl->GeoTransform[5] = adfGeoTransform[5];
		if (got_R) {			/* Need to change those */
			Ctrl->GeoTransform[0] = dfULX;
			Ctrl->GeoTransform[3] = dfULY;
		}
	}

	/* ------------------------------------------------------------------------- */
	/* Get driver information */
	/* ------------------------------------------------------------------------- */
	hDriver = GDALGetDatasetDriver (hDataset);

	Ctrl->DriverShortName = GDALGetDriverShortName (hDriver);
	Ctrl->DriverLongName  = GDALGetDriverLongName (hDriver);

	if (!got_R) {		/* That is, if we are using the entire dataset */
		Ctrl->RasterXsize = GDALGetRasterXSize (hDataset);
		Ctrl->RasterYsize = GDALGetRasterYSize (hDataset);
	}
	else if (got_R && nXSize == 0 && nYSize == 0) {		/* metadata_only */
		int	anSrcWin[4];
		anSrcWin[0] = anSrcWin[1] = anSrcWin[2] = anSrcWin[3] = 0;
		if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 ) {
			GMT_report (C, GMT_MSG_NORMAL, "The -projwin option was used, but the geotransform is rotated."
							" This configuration is not supported.\n");
			GDALClose( hDataset );
			GDALDestroyDriverManager();
			GMT_report (C, GMT_MSG_NORMAL, "Quiting with error\n");
			return(-1);
		}

		anSrcWin[0] = (int) ((dfULX - adfGeoTransform[0]) / adfGeoTransform[1] + 0.001);
		anSrcWin[1] = (int) ((dfULY - adfGeoTransform[3]) / adfGeoTransform[5] + 0.001);
		anSrcWin[2] = (int) ((dfLRX - dfULX) / adfGeoTransform[1] + 0.5);
		anSrcWin[3] = (int) ((dfLRY - dfULY) / adfGeoTransform[5] + 0.5);

		if( anSrcWin[0] < 0 || anSrcWin[1] < 0
			|| anSrcWin[0] + anSrcWin[2] > GDALGetRasterXSize(hDataset)
			|| anSrcWin[1] + anSrcWin[3] > GDALGetRasterYSize(hDataset) ) {
			GMT_report (C, GMT_MSG_NORMAL, "Computed -srcwin falls outside raster size of %dx%d.\n",
							GDALGetRasterXSize(hDataset), GDALGetRasterYSize(hDataset) );
			GMT_report (C, GMT_MSG_NORMAL, "Quiting with error\n");
			return(-1);
		}
		Ctrl->RasterXsize = nXSize = anSrcWin[2];
		Ctrl->RasterYsize = nYSize = anSrcWin[3];
	}
	else {
		Ctrl->RasterXsize = nXSize;
		Ctrl->RasterYsize = nYSize;
	}

	Ctrl->RasterCount = raster_count = GDALGetRasterCount( hDataset );

	/* ------------------------------------------------------------------------- */
	/* Get some metadata for each band. */
	/* ------------------------------------------------------------------------- */

	Ctrl->band_field_names = GMT_memory(C, NULL, raster_count, struct GDAL_BAND_FNAMES );

	/* ==================================================================== */
	/*      Loop over bands.                                                */
	/* ==================================================================== */
	for ( nBand = 0; nBand < raster_count; ++nBand ) {

		hBand = GDALGetRasterBand( hDataset, nBand+1 );

		if (!got_R) {		/* Not sure about what will realy happen in this case */
			Ctrl->band_field_names[nBand].XSize = GDALGetRasterBandXSize( hBand );
			Ctrl->band_field_names[nBand].YSize = GDALGetRasterBandYSize( hBand );
		}
		else {
			Ctrl->band_field_names[nBand].XSize = nXSize;
			Ctrl->band_field_names[nBand].YSize = nYSize;
		}

		Ctrl->band_field_names[nBand].DataType = strdup( GDALGetDataTypeName( GDALGetRasterDataType ( hBand )) );
		Ctrl->band_field_names[nBand].nodata = (double)GDALGetRasterNoDataValue( hBand, &status );

		/* Get band's Min/Max. If the band has no record of it we won't compute it */
		adfMinMax[0] = GDALGetRasterMinimum( hBand, &bGotMin );
		adfMinMax[1] = GDALGetRasterMaximum( hBand, &bGotMax );
		if ( bGotMin && bGotMax ) {
			Ctrl->band_field_names[nBand].MinMax[0] = adfMinMax[0];
			Ctrl->band_field_names[nBand].MinMax[1] = adfMinMax[1];
		}
		else {		/* No Min/Max, we won't computer it either */
			Ctrl->band_field_names[nBand].MinMax[0] = C->session.d_NaN;
			Ctrl->band_field_names[nBand].MinMax[1] = C->session.d_NaN;
		}

		/* Get band's Scale/Offset. If the band does not have them use the neutral 1/0 values */
		if (GDALGetRasterScale( hBand, &bSuccess ) != 1 || GDALGetRasterOffset( hBand, &bSuccess ) != 0) {
			Ctrl->band_field_names[nBand].ScaleOffset[0] = GDALGetRasterScale ( hBand, &bSuccess );
			Ctrl->band_field_names[nBand].ScaleOffset[1] = GDALGetRasterOffset( hBand, &bSuccess );
		}
		else {
			Ctrl->band_field_names[nBand].ScaleOffset[0] = 1.0;
			Ctrl->band_field_names[nBand].ScaleOffset[1] = 0.0;
		}

		/* Very soft guess if have grid or pixel registration. This will be confirmed latter on main */
		if (nBand == 0) pixel_reg = (GDALGetRasterDataType(hBand) == GDT_Byte);

		/* Here the Mirone code has a chunk to read overviews info, but since we have not
		   yet any use for it I won't implement that just yet.

		   Continuing the lazy path, and according to the same excuse that we don't have yet
		   any use for it, we won't check if individual bands have different colormaps.
		   Note, however, that first band colormap is captured anyway (if exists, off course) */

	}

	/* ------------------------------------------------------------------------- */
	/* Get the Color Interpretation Name */
	/* ------------------------------------------------------------------------- */
	hBand = GDALGetRasterBand( hDataset, 1 );
	if (raster_count > 0)
		Ctrl->ColorInterp = GDALGetColorInterpretationName( GDALGetRasterColorInterpretation(hBand) );
	else
		Ctrl->ColorInterp = NULL;

	/* ------------------------------------------------------------------------- */
	/* Get the first band NoData Value */
	/* ------------------------------------------------------------------------- */
	nodata = (double) (GDALGetRasterNoDataValue ( hBand, &status ) );
	if ( status == 0 )
		Ctrl->nodata = nodata;
	else
		Ctrl->nodata = 0;	/* How the hell I set a novalue-nodatavalue ? */

	/* ------------------------------------------------------------------------- */
	/* Get the Color Map of first band (if any) */
	/* ------------------------------------------------------------------------- */
	if( GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex
		&& (hTable = GDALGetRasterColorTable( hBand )) != NULL ) {

		n_colors = GDALGetColorEntryCount( hTable );
		Ctrl->ColorMap = GMT_memory(C, NULL, n_colors*4, int);
		for (i = 0, j = 0; i < n_colors; i++) {
			GDALGetColorEntryAsRGB( hTable, i, &sEntry );
			Ctrl->ColorMap[j++] = sEntry.c1;
			Ctrl->ColorMap[j++] = sEntry.c2;
			Ctrl->ColorMap[j++] = sEntry.c3;
			Ctrl->ColorMap[j++] = sEntry.c4;
		}
	}
	else
		Ctrl->ColorMap = NULL;

	/* ------------------------------------------------------------------------- */
	/* Record corners. */
	/* ------------------------------------------------------------------------- */
	if (!got_R)					/* Lower Left */
		ReportCorner (C, hDataset, 0.0, GDALGetRasterYSize(hDataset), xy_c, xy_geo[0]);
	else
		xy_c[0] = dfULX, xy_c[1] = dfLRY;
	Ctrl->Corners.LL[0] = Ctrl->hdr[0] = xy_c[0];	/* xmin, ymin */
	Ctrl->Corners.LL[1] = Ctrl->hdr[2] = xy_c[1];

	if (!got_R)					/* Upper Left */
		ReportCorner (C, hDataset, 0.0, 0.0, xy_c, xy_geo[1]);
	else
		xy_c[0] = dfULX, xy_c[1] = dfULY;
	Ctrl->Corners.UL[0] = xy_c[0];
	Ctrl->Corners.UL[1] = xy_c[1];

	if (!got_R)					/* Upper Right */
		ReportCorner (C, hDataset, GDALGetRasterXSize(hDataset), 0.0, xy_c, xy_geo[2]);
	else
		xy_c[0] = dfLRX, xy_c[1] = dfULY;
	Ctrl->Corners.UR[0] = Ctrl->hdr[1] = xy_c[0];	/* xmax, ymax */
	Ctrl->Corners.UR[1] = Ctrl->hdr[3] = xy_c[1];

	if (!got_R)					/* Lower Right */
		ReportCorner (C, hDataset, GDALGetRasterXSize(hDataset), GDALGetRasterYSize(hDataset), xy_c, xy_geo[3]);
	else
		xy_c[0] = dfLRX, xy_c[1] = dfLRY;
	Ctrl->Corners.LR[0] = xy_c[0];
	Ctrl->Corners.LR[1] = xy_c[1];

	/* --------------------------------------------------------------------------------------
	 * Record Geographical corners (if they exist)
	 * -------------------------------------------------------------------------------------- */
	if(!got_R) {
		if (!GMT_is_dnan(xy_geo[0][0])) {
			Ctrl->GEOGCorners.LL[0] = xy_geo[0][0]; Ctrl->GEOGCorners.LL[1] = xy_geo[0][1];
			Ctrl->GEOGCorners.UL[0] = xy_geo[1][0]; Ctrl->GEOGCorners.UL[1] = xy_geo[1][1];
			Ctrl->GEOGCorners.UR[0] = xy_geo[2][0]; Ctrl->GEOGCorners.UR[1] = xy_geo[2][1];
			Ctrl->GEOGCorners.LR[0] = xy_geo[3][0]; Ctrl->GEOGCorners.LR[1] = xy_geo[3][1];
		}
		else
			Ctrl->GEOGCorners.LL[0] = Ctrl->GEOGCorners.LL[1] = C->session.d_NaN;
	}
	else
		Ctrl->GEOGCorners.LL[0] = Ctrl->GEOGCorners.LL[1] = C->session.d_NaN;

	/* ------------------------------------------------------------------------- */
	/* Fill in the rest of the GMT header values (If ...) */
	if (raster_count > 0) {
		if (z_min == 1e50) {		/* We don't know yet the dataset Min/Max */
			/* If file is a "VRT/Virtual Raster" do NOT try to compute min/max and trust on XML info */
			if (strcmp(Ctrl->DriverShortName, "VRT"))
				GDALComputeRasterMinMax( hBand, false, adfMinMax );		/* NO VRT, scan file to compute min/max */
			else
				GDALComputeRasterMinMax( hBand, true, adfMinMax );		/* VRT, believe in metadata info */
			Ctrl->hdr[4] = adfMinMax[0];
			Ctrl->hdr[5] = adfMinMax[1];
		}
		else {
			Ctrl->hdr[4] = z_min;
			Ctrl->hdr[5] = z_max;
		}

		Ctrl->hdr[6] = pixel_reg;
		Ctrl->hdr[7] = adfGeoTransform[1];
		Ctrl->hdr[8] = fabs(adfGeoTransform[5]);

		if (Ctrl->hdr[2] > Ctrl->hdr[3]) {	/* Sometimes GDAL does it: y_min > y_max. If so, revert it */
			tmpdble = Ctrl->hdr[2];
			Ctrl->hdr[2] = Ctrl->hdr[3];
			Ctrl->hdr[3] = tmpdble;
		}

		if (got_R) {
			Ctrl->hdr[0] = dfULX;	Ctrl->hdr[1] = dfLRX;
			Ctrl->hdr[2] = dfLRY;	Ctrl->hdr[3] = dfULY;
		}
	}

	GDALClose (hDataset);

	return (GMT_NOERROR);
}

/************************************************************************/
/*                        ReportCorner()                                */
/************************************************************************/

int ReportCorner (struct GMT_CTRL *C, GDALDatasetH hDataset, double x, double y, double *xy_c, double *xy_geo) {
	double	dfGeoX, dfGeoY;
	const char  *pszProjection = NULL;
	double	adfGeoTransform[6];
	OGRCoordinateTransformationH hTransform = NULL;
	OGRSpatialReferenceH hProj, hLatLong = NULL;

	xy_geo[0] = xy_geo[1] = C->session.d_NaN;		/* Default return values */
/* -------------------------------------------------------------------- */
/*      Transform the point into georeferenced coordinates.             */
/* -------------------------------------------------------------------- */
	if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ) {
		pszProjection = GDALGetProjectionRef(hDataset);
		if (!strcmp(GDALGetDriverShortName(GDALGetDatasetDriver(hDataset)),"netCDF") && GDAL_VERSION_NUM <= 1450) {
			adfGeoTransform[3] *= -1;
			adfGeoTransform[5] *= -1;
		}
		dfGeoX = adfGeoTransform[0] + adfGeoTransform[1] * x + adfGeoTransform[2] * y;
		dfGeoY = adfGeoTransform[3] + adfGeoTransform[4] * x + adfGeoTransform[5] * y;
	}
	else {
		xy_c[0] = x;	xy_c[1] = y;
		return false;
	}

/* -------------------------------------------------------------------- */
/*      Report the georeferenced coordinates.                           */
/* -------------------------------------------------------------------- */
	xy_c[0] = dfGeoX;	xy_c[1] = dfGeoY;

/* -------------------------------------------------------------------- */
/*      Setup transformation to lat/long.                               */
/* -------------------------------------------------------------------- */
	if( pszProjection != NULL && strlen(pszProjection) > 0 ) {

		hProj = OSRNewSpatialReference( pszProjection );
		if( hProj != NULL ) hLatLong = OSRCloneGeogCS( hProj );

		if( hLatLong != NULL ) {
			CPLPushErrorHandler( CPLQuietErrorHandler );
			hTransform = OCTNewCoordinateTransformation( hProj, hLatLong );
			CPLPopErrorHandler();
			OSRDestroySpatialReference( hLatLong );
		}

		if( hProj != NULL ) OSRDestroySpatialReference( hProj );
	}
/*
 * --------------------------------------------------------------------
 *      Transform to latlong and report.
 * --------------------------------------------------------------------
 */
	if( hTransform != NULL && OCTTransform(hTransform,1,&dfGeoX,&dfGeoY,NULL) )
		xy_geo[0] = dfGeoX;	xy_geo[1] = dfGeoY;

	if( hTransform != NULL )
		OCTDestroyCoordinateTransformation( hTransform );

	return true;
}

/* ---------------------------------------------------------------------------------
 * record_geotransform:
 *
 * If the gdal file is not internally georeferenced, try to get the world file.
 * Returns -1 in case no world file is found.
 */
int record_geotransform ( char *gdal_filename, GDALDatasetH hDataset, double *adfGeoTransform ) {
	int status = -1;
	char generic_buffer[5000];

	if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
		return (0);

	/* Try a world file.  First the generic extension. If the gdal_filename
	   is, say, "a.tif", then this will look for "a.wld". */
	if ( GDALReadWorldFile ( gdal_filename, "wld", adfGeoTransform ) )
		return (0);

	/* Try again, but try "a.tif.wld" instead. */
	sprintf ( generic_buffer, "%s.xxx", gdal_filename );
	status = GDALReadWorldFile ( generic_buffer, "wld", adfGeoTransform );
	if (status == 1)
		return (0);

	return (-1);
}

/* -------------------------------------------------------------------- */
void ComputeRasterMinMax(struct GMT_CTRL *C, unsigned char *tmp, GDALRasterBandH hBand, double adfMinMax[2],
			int nXSize, int nYSize, double z_min, double z_max) {
	/* Compute Min/Max of a sub-region. I'm forced to do this because the
	GDALComputeRasterMinMax works only on the entire dataset */
	int	i, bGotNoDataValue;
	int16_t	tmpI16;
	uint16_t	tmpUI16;
	int32_t	tmpI32;
	uint32_t	tmpUI32;
	float	tmpF32;
	double	dfNoDataValue, tmpF64;

	dfNoDataValue = GDALGetRasterNoDataValue(hBand, &bGotNoDataValue);
	switch( GDALGetRasterDataType(hBand) ) {
		case GDT_Byte:
			for (i = 0; i < nXSize*nYSize; i++) {
				if( bGotNoDataValue && tmp[i] == dfNoDataValue )
					continue;
				z_min = MIN(tmp[i], z_min);
				z_max = MAX(tmp[i], z_max);
			}
			break;
		case GDT_Int16:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpI16, &tmp[i * sizeof(int16_t)], sizeof(int16_t));
				if( bGotNoDataValue && tmpI16 == dfNoDataValue )
					continue;
				z_min = MIN(tmpI16, z_min);
				z_max = MAX(tmpI16, z_max);
			}
			break;
		case GDT_UInt16:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpUI16, &tmp[i * sizeof(uint16_t)], sizeof(uint16_t));
				if( bGotNoDataValue && tmpUI16 == dfNoDataValue )
					continue;
				z_min = MIN(tmpUI16, z_min);
				z_max = MAX(tmpUI16, z_max);
			}
			break;
		case GDT_Int32:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpI32, &tmp[i * sizeof(int32_t)], sizeof(int32_t));
				if( bGotNoDataValue && tmpI32 == dfNoDataValue )
					continue;
				z_min = MIN(tmpI32, z_min);
				z_max = MAX(tmpI32, z_max);
			}
			break;
		case GDT_UInt32:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpUI32, &tmp[i * sizeof(uint32_t)], sizeof(uint32_t));
				if( bGotNoDataValue && tmpUI32 == dfNoDataValue )
					continue;
				z_min = MIN(tmpUI32, z_min);
				z_max = MAX(tmpUI32, z_max);
			}
			break;
		case GDT_Float32:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpF32, &tmp[i * sizeof(float)], sizeof(float));
				if( bGotNoDataValue && tmpF32 == dfNoDataValue )
					continue;
				z_min = MIN(tmpF32, z_min);
				z_max = MAX(tmpF32, z_max);
			}
			break;
		case GDT_Float64:
			for (i = 0; i < nXSize*nYSize; i++) {
				memcpy (&tmpF64, &tmp[i * sizeof(double)], sizeof(double));
				if( bGotNoDataValue && tmpF64 == dfNoDataValue )
					continue;
				z_min = MIN(tmpF64, z_min);
				z_max = MAX(tmpF64, z_max);
			}
			break;
		default:
			GMT_report (C, GMT_MSG_NORMAL, "gdalread: Unsupported data type\n");
			break;
	}
	adfMinMax[0] = z_min;
	adfMinMax[1] = z_max;
}

/* -------------------------------------------------------------------- */
int gdal_decode_columns (struct GMT_CTRL *GMT, char *txt, int *whichBands, unsigned int n_col) {
	unsigned int n = 0, i, start, stop, pos = 0;
	char p[GMT_BUFSIZ];

	while ((GMT_strtok (txt, ",", &pos, p))) {
		if (strchr (p, '-'))
			sscanf (p, "%d-%d", &start, &stop);
		else {
			sscanf (p, "%d", &start);
			stop = start;
		}
		stop = MIN (stop, n_col);
		for (i = start; i <= stop; i++) {
			whichBands[n] = i + 1;			/* Band numbering in GMT is 0 based */
			n++;
		}
	}
	return ((int)n);
}
