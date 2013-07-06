/* $Id$
 *
 * Copyright (c) 2012-2013
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_core_module.c populates the external array of GMT core
 * module parameters such as name, group and purpose strings.
 * This file also contains the following convenience function to
 * display all module purposes:
 *
 *   void gmt_core_module_show_all (struct GMTAPI_CTRL *API);
 *
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt
 * and regenerate this file with gmt_make_module_src.sh core */

#include "gmt.h"
#include <string.h>

/* Sorted array with information for all GMT core modules */

/* name, library, and purpose for each module */
struct Gmt_moduleinfo {
	const char *name;             /* Program name */
	const char *component;        /* Component (core, supplement, custom) */
	const char *purpose;          /* Program purpose */
};

struct Gmt_moduleinfo g_core_module[] = {
	{"blockmean", "core", "Block average (x,y,z) data tables by L2 norm"},
	{"blockmedian", "core", "Block average (x,y,z) data tables by L1 norm (spatial median)"},
	{"blockmode", "core", "Block average (x,y,z) data tables by mode estimation"},
	{"colmath", "core", "Do mathematics on columns from data tables"},
	{"filter1d", "core", "Do time domain filtering of 1-D data tables"},
	{"fitcircle", "core", "Find mean position and best-fitting great- or small-circle to points on sphere"},
	{"gmt2kml", "core", "Convert GMT data tables to KML files for Google Earth"},
	{"gmtconnect", "core", "Connect individual lines whose end points match within tolerance"},
	{"gmtconvert", "core", "Convert, paste, or extract columns from data tables"},
	{"gmtdefaults", "core", "List current GMT default parameters"},
	{"gmtget", "core", "Get individual GMT default parameters"},
	{"gmtmath", "core", "Reverse Polish Notation (RPN) calculator for data tables"},
	{"gmtread", "core", "Read GMT objects into external API"},
	{"gmtselect", "core", "Select data table subsets based on multiple spatial criteria"},
	{"gmtset", "core", "Change individual GMT default parameters"},
	{"gmtsimplify", "core", "Line reduction using the Douglas-Peucker algorithm"},
	{"gmtspatial", "core", "Do geospatial operations on lines and polygons"},
	{"gmtvector", "core", "Basic manipulation of Cartesian vectors"},
	{"gmtwhich", "core", "Find full path to specified files"},
	{"gmtwrite", "core", "Write GMT objects from external API"},
	{"grd2cpt", "core", "Make linear or histogram-equalized color palette table from grid"},
	{"grd2rgb", "core", "Write r/g/b grid files from a grid file, a raw RGB file, or SUN rasterfile"},
	{"grd2xyz", "core", "Convert grid file to data table"},
	{"grdblend", "core", "Blend several partially over-lapping grids into one larger grid"},
	{"grdclip", "core", "Clip the range of grids"},
	{"grdcontour", "core", "Make contour map using a grid"},
	{"grdcut", "core", "Extract subregion from a grid"},
	{"grdedit", "core", "Modify header or content of a grid"},
	{"grdfft", "core", "Do mathematical operations on grids in the wavenumber (or frequency) domain"},
	{"grdfilter", "core", "Filter a grid in the space (or time) domain"},
	{"grdgradient", "core", "Compute directional gradients from a grid"},
	{"grdhisteq", "core", "Perform histogram equalization for a grid"},
	{"grdimage", "core", "Project grids or images and plot them on maps"},
	{"grdinfo", "core", "Extract information from grids"},
	{"grdlandmask", "core", "Create a \"wet-dry\" mask grid from shoreline data base"},
	{"grdmask", "core", "Create mask grid from polygons or point coverage"},
	{"grdmath", "core", "Reverse Polish Notation (RPN) calculator for grids (element by element)"},
	{"grdpaste", "core", "Join two grids along their common edge"},
	{"grdproject", "core", "Forward and inverse map transformation of grids"},
	{"grdraster", "core", "Extract subregion from a binary raster and save as a GMT grid"},
	{"grdreformat", "core", "Convert between different grid formats"},
	{"grdsample", "core", "Resample a grid onto a new lattice"},
	{"grdtrack", "core", "Sample grids at specified (x,y) locations"},
	{"grdtrend", "core", "Fit trend surface to grids and compute residuals"},
	{"grdvector", "core", "Plot vector field from two component grids"},
	{"grdview", "core", "Create 3-D perspective image or surface mesh from a grid"},
	{"grdvolume", "core", "Calculate grid volume and area constrained by a contour"},
	{"greenspline", "core", "Interpolate using Green's functions for splines in 1-3 dimensions"},
	{"kml2gmt", "core", "Extract GMT table data from Google Earth KML files"},
	{"makecpt", "core", "Make GMT color palette tables"},
	{"mapproject", "core", "Do forward and inverse map transformations, datum conversions and geodesy"},
	{"minmax", "core", "Find extreme values in data tables"},
	{"nearneighbor", "core", "Grid table data using a \"Nearest neighbor\" algorithm"},
	{"project", "core", "Project table data onto lines or great circles, generate tracks, or translate coordinates"},
	{"ps2raster", "core", "Convert [E]PS file(s) to other formats using GhostScript"},
	{"psbasemap", "core", "Plot PostScript base maps"},
	{"psclip", "core", "Initialize or terminate polygonal clip paths"},
	{"pscoast", "core", "Plot continents, countries, shorelines, rivers, and borders on maps"},
	{"pscontour", "core", "Contour table data by direct triangulation"},
	{"pshistogram", "core", "Calculate and plot histograms"},
	{"psimage", "core", "Place images or EPS files on maps"},
	{"pslegend", "core", "Plot legends on maps"},
	{"psmask", "core", "Use data tables to clip or mask map areas with no coverage"},
	{"psrose", "core", "Plot a polar histogram (rose, sector, windrose diagrams)"},
	{"psscale", "core", "Plot a gray-scale or color-scale on maps"},
	{"pstext", "core", "Plot or typeset text on maps"},
	{"pswiggle", "core", "Plot z = f(x,y) anomalies along tracks"},
	{"psxyz", "core", "Plot lines, polygons, and symbols in 3-D"},
	{"psxy", "core", "Plot lines, polygons, and symbols on maps"},
	{"sample1d", "core", "Resample 1-D table data using splines"},
	{"spectrum1d", "core", "Compute auto- [and cross-] spectra from one [or two] timeseries"},
	{"sph2grd", "core", "Compute grid from spherical harmonic coefficients"},
	{"sphdistance", "core", "Make grid of distances to nearest points on a sphere"},
	{"sphinterpolate", "core", "Spherical gridding in tension of data on a sphere"},
	{"sphtriangulate", "core", "Delaunay or Voronoi construction of spherical lon,lat data"},
	{"splitxyz", "core", "Split xyz[dh] data tables into individual segments"},
	{"surface", "core", "Grid table data using adjustable tension continuous curvature splines"},
	{"trend1d", "core", "Fit a [weighted] [robust] polynomial [or Fourier] model for y = f(x) to xy[w] data"},
	{"trend2d", "core", "Fit a [weighted] [robust] polynomial for z = f(x,y) to xyz[w] data"},
	{"triangulate", "core", "Do optimal (Delaunay) triangulation and gridding of Cartesian table data"},
	{"xyz2grd", "core", "Convert data table to a grid file"},
	{NULL, NULL, NULL} /* last element == NULL detects end of array */
};

/* Pretty print all GMT core module names and their purposes */
void gmt_core_module_show_all (void *API) {
	unsigned int module_id = 0;
	char message[256];

	GMT_Message (API, GMT_TIME_NONE, "\n=== " "GMT core: The main section of the Generic Mapping Tools" " ===\n");
	while (g_core_module[module_id].name != NULL) {
		if (module_id == 0 || strcmp (g_core_module[module_id-1].component, g_core_module[module_id].component)) {
			/* Start of new supplemental group */
			sprintf (message, "\nModule name:     Purpose of %s module:\n", g_core_module[module_id].component);
			GMT_Message (API, GMT_TIME_NONE, message);
			GMT_Message (API, GMT_TIME_NONE, "----------------------------------------------------------------\n");
		}
		sprintf (message, "%-16s %s\n",
			g_core_module[module_id].name, g_core_module[module_id].purpose);
		GMT_Message (API, GMT_TIME_NONE, message);
		++module_id;
	}
}