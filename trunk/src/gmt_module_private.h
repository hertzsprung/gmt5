/* $Id$
 *
 * Copyright (c) 2012-2013
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_module.h declares the array that contains GMT module parameters such as
 * name and purpose strings.
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#pragma once
#ifndef _GMT_MODULE_PRIVATE_H
#define _GMT_MODULE_PRIVATE_H

#ifdef __cplusplus /* Basic C++ support */
extern "C" {
#endif

/* CMake definitions: This must be first! */
#include "gmt_config.h"

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"

/* name, purpose, Api_mode, and function pointer for each GMT module */
struct Gmt_moduleinfo {
	const char *name;             /* Program name */
	const char *component;        /* Component (core or supplement) */
	const char *purpose;          /* Program purpose */
	/* gmt module function pointer: */
	int (*p_func)(void*, int, void*);
};

/* external array with program paramerters for all GMT modules */
EXTERN_MSC struct Gmt_moduleinfo g_module[];

EXTERN_MSC int GMT_backtracker (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_blockmean (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_blockmedian (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_blockmode (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_colmath (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_filter1d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_fitcircle (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmt2kml (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtaverage (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtconvert (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtdefaults (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtdp (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtget (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtgravmag3d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtmath (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtselect (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtset (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtspatial (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtstitch (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtvector (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gmtwhich (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gravfft (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grd2cpt (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grd2rgb (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grd2xyz (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdblend (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdclip (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdcontour (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdcut (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdedit (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdfft (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdfilter (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdgradient (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdgravmag3d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdhisteq (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdimage (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdinfo (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdlandmask (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdmask (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdmath (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdpaste (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdpmodeler (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdproject (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdraster (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdredpol (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdreformat (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdrotater (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdsample (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdseamount (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdspotter (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdtrack (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdtrend (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdvector (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdview (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_grdvolume (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_greenspline (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_gshhg (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_hotspotter (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_img2grd (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_kml2gmt (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_makecpt (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mapproject (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77convert (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77info (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77list (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77magref (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77manage (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77path (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77sniffer (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_mgd77track (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_minmax (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_nearneighbor (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_originator (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_project (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_ps2raster (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psbasemap (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psclip (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pscoast (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pscontour (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pscoupe (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pshistogram (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psimage (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pslegend (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psmask (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psmeca (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pspolar (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psrose (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psscale (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pssegy (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pssegyz (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pstext (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psvelo (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_pswiggle (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psxy (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_psxyz (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_rotconverter (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_sample1d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_segy2grd (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_spectrum1d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_sph2grd (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_sphdistance (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_sphinterpolate (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_sphtriangulate (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_splitxyz (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_surface (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_testapi (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_trend1d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_trend2d (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_triangulate (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_binlist (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_cross (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_datalist (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_get (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_init (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_list (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_merge (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_put (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_report (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_x2sys_solve (void *api_ctrl, int mode, void *args);
EXTERN_MSC int GMT_xyz2grd (void *api_ctrl, int mode, void *args);

/* Pretty print all module names and their purposes */
EXTERN_MSC void gmt_module_show_all();

/* Pretty print module names and purposes */
EXTERN_MSC void gmt_module_show_name_and_purpose(enum GMT_MODULE_ID module);

/* Lookup module id by name */
EXTERN_MSC enum GMT_MODULE_ID gmt_module_lookup (const char *candidate);

/* Get module name */
EXTERN_MSC const char *gmt_module_name (struct GMT_CTRL *gmt_ctrl);

#ifdef __cplusplus
}
#endif

#endif /* !_GMT_MODULE_PRIVATE_H */
