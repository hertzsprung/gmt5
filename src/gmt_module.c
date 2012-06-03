/* $Id$
 *
 * Copyright (c) 2012
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_module.c populates the external array of GMT module parameters such as name
 * and purpose strings, and function pointers to call the module functions.
 * This file also contains the following convenience functions to access the module
 * parameter array:
 *   void gmt_module_show_all(); - Pretty print all module names and their
 *           purposes
 *   void gmt_module_show_name_and_purpose(enum Gmt_module_id module); - Pretty
 *           print module names and purposes
 *   enum Gmt_module_id gmt_module_lookup (char *candidate); - Lookup module id by
 *           name
 *   char *gmt_module_name (struct GMT_CTRL *gmt_ctrl); - Get module name
 *
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#include <stdio.h>
#include <string.h>

#include "gmt.h"

/* sorted array with program paramerters for all GMT modules */
struct Gmt_moduleinfo g_module[] = {
	{"backtracker", "spotter", "Generate forward and backward flowlines and hotspot tracks", k_mode_gmt, &GMT_backtracker},
	{"blockmean", "core", "Block average (x,y,z) data tables by L2 norm", k_mode_gmt, &GMT_blockmean},
	{"blockmedian", "core", "Block average (x,y,z) data tables by L1 norm (spatial median)", k_mode_gmt, &GMT_blockmedian},
	{"blockmode", "core", "Block average (x,y,z) data tables by mode estimation", k_mode_gmt, &GMT_blockmode},
	{"colmath", "core", "Do mathematics on columns from data tables", k_mode_gmt, &GMT_colmath},
	{"filter1d", "core", "Do time domain filtering of 1-D data tables", k_mode_gmt, &GMT_filter1d},
	{"fitcircle", "core", "Find mean position and best-fitting great- or small-circle to points on sphere", k_mode_gmt, &GMT_fitcircle},
	{"gmt2kml", "core", "Convert GMT data tables to KML files for Google Earth", k_mode_gmt, &GMT_gmt2kml},
	{"gmtaverage", "core", "Block average (x, y, z) data tables by mean, median, or mode estimation", k_mode_gmt, &GMT_gmtaverage},
	{"gmtconvert", "core", "Convert, paste, or extract columns from data tables", k_mode_gmt, &GMT_gmtconvert},
	{"gmtdefaults", "core", "List current GMT default parameters", k_mode_psl, &GMT_gmtdefaults},
	{"gmtdp", "core", "Line reduction using the Douglas-Peucker algorithm", k_mode_gmt, &GMT_gmtdp},
	{"gmtget", "core", "Get individual GMT default parameters", k_mode_psl, &GMT_gmtget},
	{"gmtmath", "core", "Reverse Polish Notation (RPN) calculator for data tables", k_mode_gmt, &GMT_gmtmath},
	{"gmtselect", "core", "Select data table subsets based on multiple spatial criteria", k_mode_gmt, &GMT_gmtselect},
	{"gmtset", "core", "Change individual GMT default parameters", k_mode_psl, &GMT_gmtset},
	{"gmtspatial", "core", "Do geospatial operations on lines and polygons", k_mode_gmt, &GMT_gmtspatial},
	{"gmtstitch", "core", "Join individual lines whose end points match within tolerance", k_mode_gmt, &GMT_gmtstitch},
	{"gmtvector", "core", "Basic manipulation of Cartesian vectors", k_mode_gmt, &GMT_gmtvector},
	{"gmtwhich", "core", "Find full path to specified files", k_mode_gmt, &GMT_gmtwhich},
	{"gravfft", "potential", "Compute gravitational attraction of 3-D surfaces and a little more (ATTENTION z positive up)", k_mode_gmt, &GMT_gravfft},
	{"grd2cpt", "core", "Make linear or histogram-equalized color palette table from grid", k_mode_gmt, &GMT_grd2cpt},
	{"grd2rgb", "core", "Write r/g/b grid files from a grid file, a raw RGB file, or SUN rasterfile", k_mode_gmt, &GMT_grd2rgb},
	{"grd2xyz", "core", "Convert grid file to data table", k_mode_gmt, &GMT_grd2xyz},
	{"grdblend", "core", "Blend several partially over-lapping grids into one larger grid", k_mode_gmt, &GMT_grdblend},
	{"grdclip", "core", "Clip the range of grids", k_mode_gmt, &GMT_grdclip},
	{"grdcontour", "core", "Make contour map using a grid", k_mode_psl, &GMT_grdcontour},
	{"grdcut", "core", "Extract subregion from a grid", k_mode_gmt, &GMT_grdcut},
	{"grdedit", "core", "Modify header or content of a grid", k_mode_gmt, &GMT_grdedit},
	{"grdfft", "core", "Do mathematical operations on grids in the wavenumber (or frequency) domain", k_mode_gmt, &GMT_grdfft},
	{"grdfilter", "core", "Filter a grid in the space (or time) domain", k_mode_gmt, &GMT_grdfilter},
	{"grdgradient", "core", "Compute directional gradients from a grid", k_mode_gmt, &GMT_grdgradient},
	{"grdhisteq", "core", "Perform histogram equalization for a grid", k_mode_gmt, &GMT_grdhisteq},
	{"grdimage", "core", "Project grids or images and plot them on maps", k_mode_psl, &GMT_grdimage},
	{"grdinfo", "core", "Extract information from grids", k_mode_gmt, &GMT_grdinfo},
	{"grdlandmask", "core", "Create a \"wet-dry\" mask grid from shoreline data base", k_mode_gmt, &GMT_grdlandmask},
	{"grdmask", "core", "Create mask grid from polygons or point coverage", k_mode_gmt, &GMT_grdmask},
	{"grdmath", "core", "Reverse Polish Notation (RPN) calculator for grids (element by element)", k_mode_gmt, &GMT_grdmath},
	{"grdokb", "potential", "Computes the gravity effect of one (or two) grids by the method of Okabe", k_mode_gmt, &GMT_grdokb},
	{"grdpaste", "core", "Join two grids along their common edge", k_mode_gmt, &GMT_grdpaste},
	{"grdpmodeler", "spotter", "Evaluate a plate model on a geographic grid", k_mode_gmt, &GMT_grdpmodeler},
	{"grdproject", "core", "Forward and inverse map transformation of grids", k_mode_gmt, &GMT_grdproject},
	{"grdraster", "dbase", "Extract subregion from a binary raster and save as a GMT grid", k_mode_gmt, &GMT_grdraster},
	{"grdreformat", "core", "Convert between different grid formats", k_mode_gmt, &GMT_grdreformat},
	{"grdrotater", "spotter", "Finite rotation reconstruction of geographic grid", k_mode_gmt, &GMT_grdrotater},
	{"grdsample", "core", "Resample a grid onto a new lattice", k_mode_gmt, &GMT_grdsample},
	{"grdspotter", "spotter", "Create CVA image from a gravity or topography grid", k_mode_gmt, &GMT_grdspotter},
	{"grdtrack", "core", "Sample grids at specified (x,y) locations", k_mode_gmt, &GMT_grdtrack},
	{"grdtrend", "core", "Fit trend surface to grids and compute residuals", k_mode_gmt, &GMT_grdtrend},
	{"grdvector", "core", "Plot vector field from two component grids", k_mode_psl, &GMT_grdvector},
	{"grdview", "core", "Create 3-D perspective image or surface mesh from a grid", k_mode_psl, &GMT_grdview},
	{"grdvolume", "core", "Calculate grid volume and area constrained by a contour", k_mode_gmt, &GMT_grdvolume},
	{"greenspline", "core", "Interpolate using Green's functions for splines in 1-3 dimensions", k_mode_gmt, &GMT_greenspline},
	{"gshhg", "gshhg", "Extract data tables from binary GSHHS or WDBII data files", k_mode_gmt, &GMT_gshhg},
	{"hotspotter", "spotter", "Create CVA image from seamount locations", k_mode_gmt, &GMT_hotspotter},
	{"img2grd", "imgsrc", "Extract a subset from an img file in Mercator or Geographic format", k_mode_gmt, &GMT_img2grd},
	{"kml2gmt", "core", "Extract GMT table data from Google Earth KML files", k_mode_gmt, &GMT_kml2gmt},
	{"makecpt", "core", "Make GMT color palette tables", k_mode_gmt, &GMT_makecpt},
	{"mapproject", "core", "Do forward and inverse map transformations, datum conversions and geodesy", k_mode_gmt, &GMT_mapproject},
	{"mgd77convert", "mgd77", "Convert MGD77 data to other file formats", k_mode_gmt, &GMT_mgd77convert},
	{"mgd77info", "mgd77", "Extract information about MGD77 files", k_mode_gmt, &GMT_mgd77info},
	{"mgd77list", "mgd77", "Extract data from MGD77 files", k_mode_gmt, &GMT_mgd77list},
	{"mgd77magref", "mgd77", "Evaluate the IGRF or CM4 magnetic field models", k_mode_gmt, &GMT_mgd77magref},
	{"mgd77manage", "mgd77", "Manage the content of MGD77+ files", k_mode_gmt, &GMT_mgd77manage},
	{"mgd77path", "mgd77", "Return paths to MGD77 cruises and directories", k_mode_gmt, &GMT_mgd77path},
	{"mgd77sniffer", "mgd77", "Along-track quality control of MGD77 cruises", k_mode_gmt, &GMT_mgd77sniffer},
	{"mgd77track", "mgd77", "Plot track-line map of MGD77 cruises", k_mode_psl, &GMT_mgd77track},
	{"minmax", "core", "Find extreme values in data tables", k_mode_gmt, &GMT_minmax},
	{"nearneighbor", "core", "Grid table data using a \"Nearest neighbor\" algorithm", k_mode_gmt, &GMT_nearneighbor},
	{"originator", "spotter", "Associate seamounts with nearest hotspot point sources", k_mode_gmt, &GMT_originator},
	{"project", "core", "Project table data onto lines or great circles, generate tracks, or translate coordinates", k_mode_gmt, &GMT_project},
	{"ps2raster", "core", "Convert [E]PS file(s) to other formats using GhostScript.", k_mode_gmt, &GMT_ps2raster},
	{"psbasemap", "core", "Plot PostScript base maps", k_mode_psl, &GMT_psbasemap},
	{"psclip", "core", "Initialize or terminate polygonal clip paths", k_mode_psl, &GMT_psclip},
	{"pscoast", "core", "Plot continents, shorelines, rivers, and borders on maps", k_mode_psl, &GMT_pscoast},
	{"pscontour", "core", "Contour table data by direct triangulation", k_mode_psl, &GMT_pscontour},
	{"pscoupe", "meca", "Plot cross-sections of focal mechanisms", k_mode_psl, &GMT_pscoupe},
	{"pshistogram", "core", "Calculate and plot histograms", k_mode_psl, &GMT_pshistogram},
	{"psimage", "core", "Place images or EPS files on maps", k_mode_psl, &GMT_psimage},
	{"pslegend", "core", "Plot legends on maps", k_mode_psl, &GMT_pslegend},
	{"psmask", "core", "Use data tables to clip or mask map areas with no coverage", k_mode_psl, &GMT_psmask},
	{"psmeca", "meca", "Plot focal mechanisms on maps", k_mode_psl, &GMT_psmeca},
	{"pspolar", "meca", "Plot polarities on the inferior focal half-sphere on maps", k_mode_psl, &GMT_pspolar},
	{"psrose", "core", "Plot a polar histogram (rose, sector, windrose diagrams)", k_mode_psl, &GMT_psrose},
	{"psscale", "core", "Plot a gray-scale or color-scale on maps", k_mode_psl, &GMT_psscale},
	{"pssegy", "segy", "Plot a SEGY file on a map", k_mode_psl, &GMT_pssegy},
	{"pssegyz", "segy", "Plot a SEGY file in PostScript", k_mode_psl, &GMT_pssegyz},
	{"pstext", "core", "Plot or typeset text on maps", k_mode_psl, &GMT_pstext},
	{"psvelo", "meca", "Plot velocity vectors, crosses, and wedges on maps", k_mode_psl, &GMT_psvelo},
	{"pswiggle", "core", "Plot z = f(x,y) anomalies along tracks", k_mode_psl, &GMT_pswiggle},
	{"psxy", "core", "Plot lines, polygons, and symbols on maps", k_mode_psl, &GMT_psxy},
	{"psxyz", "core", "Plot lines, polygons, and symbols in 3-D", k_mode_psl, &GMT_psxyz},
	{"redpol", "potential", "Compute the Continuous Reduction To the Pole, AKA differential RTP", k_mode_gmt, &GMT_redpol},
	{"rotconverter", "spotter", "Manipulate total reconstruction and stage rotations", k_mode_gmt, &GMT_rotconverter},
	{"sample1d", "core", "Resample 1-D table data using splines", k_mode_gmt, &GMT_sample1d},
	{"segy2grd", "segy", "Converting SEGY data to a GMT grid", k_mode_gmt, &GMT_segy2grd},
	{"spectrum1d", "core", "Compute auto- [and cross- ] spectra from one [or two] timeseries", k_mode_gmt, &GMT_spectrum1d},
	{"sphdistance", "sph", "Make grid of distances to nearest points on a sphere", k_mode_gmt, &GMT_sphdistance},
	{"sphinterpolate", "sph", "Spherical gridding in tension of data on a sphere", k_mode_gmt, &GMT_sphinterpolate},
	{"sphtriangulate", "sph", "Delaunay or Voronoi construction of spherical lon,lat data", k_mode_gmt, &GMT_sphtriangulate},
	{"splitxyz", "core", "Split xyz[dh] data tables into individual segments", k_mode_gmt, &GMT_splitxyz},
	{"surface", "core", "Grid table data using adjustable tension continuous curvature splines", k_mode_gmt, &GMT_surface},
	{"testapi", "core", "test API i/o methods for any data type", k_mode_psl, &GMT_testapi},
	{"trend1d", "core", "Fit a [weighted] [robust] polynomial [or Fourier] model for y = f(x) to xy[w] data", k_mode_gmt, &GMT_trend1d},
	{"trend2d", "core", "Fit a [weighted] [robust] polynomial for z = f(x,y) to xyz[w] data", k_mode_gmt, &GMT_trend2d},
	{"triangulate", "core", "Do optimal (Delaunay) triangulation and gridding of Cartesian table data", k_mode_gmt, &GMT_triangulate},
	{"x2sys_binlist", "x2sys", "Create bin index listing from track data files", k_mode_gmt, &GMT_x2sys_binlist},
	{"x2sys_cross", "x2sys", "Calculate crossovers between track data files", k_mode_gmt, &GMT_x2sys_cross},
	{"x2sys_datalist", "x2sys", "Extract content of track data files", k_mode_gmt, &GMT_x2sys_datalist},
	{"x2sys_get", "x2sys", "Get track listing from track index database", k_mode_gmt, &GMT_x2sys_get},
	{"x2sys_init", "x2sys", "Initialize a new x2sys track database", k_mode_gmt, &GMT_x2sys_init},
	{"x2sys_list", "x2sys", "Extract subset from crossover data base", k_mode_gmt, &GMT_x2sys_list},
	{"x2sys_merge", "x2sys", "Merge an updated COEs table (smaller) into the main table (bigger)", k_mode_gmt, &GMT_x2sys_merge},
	{"x2sys_put", "x2sys", "Update track index database from track bin file", k_mode_gmt, &GMT_x2sys_put},
	{"x2sys_report", "x2sys", "Report statistics from crossover data base", k_mode_gmt, &GMT_x2sys_report},
	{"x2sys_solve", "x2sys", "Determine least-squares systematic correction from crossovers", k_mode_gmt, &GMT_x2sys_solve},
	{"xyz2grd", "core", "Convert data table to a grid file", k_mode_gmt, &GMT_xyz2grd},
	{"xyzokb", "potential", "Compute the gravity/magnetic anomaly of a body by the method of Okabe", k_mode_gmt, &GMT_xyzokb},
  {NULL, NULL, NULL, -1, NULL} /* last element == NULL detects end of array */
};

/* Pretty print all module names and their purposes */
void gmt_module_show_all() {
	enum Gmt_module_id module_id = 0; /* Module ID */

	fprintf (stderr, "Program - Purpose of Program\n");
	while (g_module[module_id].name != NULL) {
		fprintf (stderr, "%s(%s) - %s\n",
				g_module[module_id].name,
				g_module[module_id].component,
				g_module[module_id].purpose);
		++module_id;
	}
}

/* Pretty print module names and purposes */
void gmt_module_show_name_and_purpose(enum Gmt_module_id module_id) {
	fprintf (stderr, "%s(%s) %s - %s\n\n",
			g_module[module_id].name,
			g_module[module_id].component,
			GMT_version(),
			g_module[module_id].purpose);
}

/* Lookup module id by name */
enum Gmt_module_id gmt_module_lookup (char *candidate) {
	enum Gmt_module_id module_id = 0; /* Module ID */

	/* Match candidate against g_module[module_id].name */
	while ( g_module[module_id].name != NULL &&
			strcmp (candidate, g_module[module_id].name) )
		++module_id;

	/* Return matching Module ID or k_mod_nongmt */
	return module_id;
}

/* Get module name */
char *gmt_module_name (struct GMT_CTRL *gmt_ctrl) {
	char *module_name;
	module_name = gmt_ctrl->init.module_id == k_mod_nongmt ?
			gmt_ctrl->init.module_name : g_module[gmt_ctrl->init.module_id].name;
	return module_name;
}
