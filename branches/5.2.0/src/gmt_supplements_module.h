/* $Id$
 *
 * Copyright (c) 2012-2015
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_supplements_module.h declares the prototypes for supplements module functions
 * and the array that contains supplements GMT module parameters such as name
 * and purpose strings.
 * DO NOT edit this file directly! Regenerate thee file by running
 * 	gmt_make_module_src.sh supplements
 */

#pragma once
#ifndef _GMT_SUPPLEMENTS_MODULE_H
#define _GMT_SUPPLEMENTS_MODULE_H

#ifdef __cplusplus /* Basic C++ support */
extern "C" {
#endif

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"

/* Prototypes of all modules in the GMT supplements library */
EXTERN_MSC int GMT_grd2sph (void *API, int mode, void *args);
EXTERN_MSC int GMT_gshhg (void *API, int mode, void *args);
EXTERN_MSC int GMT_img2grd (void *API, int mode, void *args);
EXTERN_MSC int GMT_pscoupe (void *API, int mode, void *args);
EXTERN_MSC int GMT_psmeca (void *API, int mode, void *args);
EXTERN_MSC int GMT_pspolar (void *API, int mode, void *args);
EXTERN_MSC int GMT_psvelo (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77convert (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77info (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77list (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77magref (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77manage (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77path (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77sniffer (void *API, int mode, void *args);
EXTERN_MSC int GMT_mgd77track (void *API, int mode, void *args);
EXTERN_MSC int GMT_dimfilter (void *API, int mode, void *args);
EXTERN_MSC int GMT_talwani2d
 (void *API, int mode, void *args);
EXTERN_MSC int GMT_talwani3d
 (void *API, int mode, void *args);
EXTERN_MSC int GMT_gmtflexure (void *API, int mode, void *args);
EXTERN_MSC int GMT_gmtgravmag3d (void *API, int mode, void *args);
EXTERN_MSC int GMT_gravfft (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdflexure (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdgravmag3d (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdredpol (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdseamount (void *API, int mode, void *args);
EXTERN_MSC int GMT_pssegyz (void *API, int mode, void *args);
EXTERN_MSC int GMT_pssegy (void *API, int mode, void *args);
EXTERN_MSC int GMT_segy2grd (void *API, int mode, void *args);
EXTERN_MSC int GMT_backtracker (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdpmodeler (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdrotater (void *API, int mode, void *args);
EXTERN_MSC int GMT_grdspotter (void *API, int mode, void *args);
EXTERN_MSC int GMT_hotspotter (void *API, int mode, void *args);
EXTERN_MSC int GMT_originator (void *API, int mode, void *args);
EXTERN_MSC int GMT_rotconverter (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_binlist (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_cross (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_datalist (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_get (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_init (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_list (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_merge (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_put (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_report (void *API, int mode, void *args);
EXTERN_MSC int GMT_x2sys_solve (void *API, int mode, void *args);

/* Pretty print all modules in the GMT supplements library and their purposes */
EXTERN_MSC void gmt_supplements_module_show_all (void *API);

/* Undocumented API function for developers to get information about a module */
EXTERN_MSC const char * gmt_supplements_module_info (void *API, char *candidate);

#ifdef __cplusplus
}
#endif

#endif /* !_GMT_SUPPLEMENTS_MODULE_H */
