/* $Id$
 *
 * Copyright (c) 2012-2015
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
 * Developers of external APIs for accessing GMT modules may use this
 * function to retrieve option keys needed for module arg processing:
 *
 *   char * gmt_core_module_info (void *API, const char *module);
 *
 * DO NOT edit this file directly! Regenerate thee file by running
 * 	gmt_make_module_src.sh core
 */
#include "gmt_dev.h"
#ifndef BUILD_SHARED_LIBS
#include "gmt_core_module.h"
#endif

static inline struct GMTAPI_CTRL * gmt_get_api_ptr (struct GMTAPI_CTRL *ptr) {return (ptr);}

/* Sorted array with information for all GMT core modules */

/* name, library, and purpose for each module */
struct Gmt_moduleinfo {
	const char *name;             /* Program name */
	const char *component;        /* Component (core, supplement, custom) */
	const char *purpose;          /* Program purpose */
	const char *keys;             /* Program option info for external APIs */
#ifndef BUILD_SHARED_LIBS
	/* gmt module function pointer: */
	int (*p_func)(void*, int, void*);
#endif
};

struct Gmt_moduleinfo g_core_module[] = {
#ifdef BUILD_SHARED_LIBS
	{"blockmean", "core", "Block average (x,y,z) data tables by L2 norm", "<DI,>DO"},
	{"blockmedian", "core", "Block average (x,y,z) data tables by L1 norm (spatial median)", "<DI,>DO"},
	{"blockmode", "core", "Block average (x,y,z) data tables by mode estimation", "<DI,>DO"},
	{"filter1d", "core", "Do time domain filtering of 1-D data tables", "<DI,>DO"},
	{"fitcircle", "core", "Find mean position and best-fitting great- or small-circle to points on sphere", "<DI,>TO"},
	{"gmt2kml", "core", "Convert GMT data tables to KML files for Google Earth", "<DI,>TO"},
	{"gmtconnect", "core", "Connect individual lines whose end points match within tolerance", "<DI,>DO,CDo,LTo,QTo"},
	{"gmtconvert", "core", "Convert, paste, or extract columns from data tables", "<DI,>DO"},
	{"gmtdefaults", "core", "List current GMT default parameters", ">TO"},
	{"gmtget", "core", "Get individual GMT default parameters", ">TO"},
	{"gmtinfo", "core", "Get information about data tables", "<DI,>DO"},
	{"gmtlogo", "core", "Plot the GMT logo on maps", ""},
	{"gmtmath", "core", "Reverse Polish Notation (RPN) calculator for data tables", "<DI,ADI,>DO"},
	{"gmtregress", "core", "Perform linear regression of 1-D data sets", "<DI,>DO"},
	{"gmtselect", "core", "Select data table subsets based on multiple spatial criteria", "<DI,CDi,FDi,LDi,>DO"},
	{"gmtset", "core", "Change individual GMT default parameters", "<TI"},
	{"gmtsimplify", "core", "Line reduction using the Douglas-Peucker algorithm", "<DI,>DO"},
	{"gmtspatial", "core", "Do geospatial operations on lines and polygons", "<DI,DDI,NDI,TDI,>DO"},
	{"gmtvector", "core", "Basic manipulation of Cartesian vectors", "<DI,ADI,>DO"},
	{"gmtwhich", "core", "Find full path to specified files", "<TI,>TO"},
	{"grd2cpt", "core", "Make linear or histogram-equalized color palette table from grid", "<GI,>CO"},
	{"grd2rgb", "core", "Write r/g/b grid files from a grid file, a raw RGB file, or SUN rasterfile", "<GI"},
	{"grd2xyz", "core", "Convert grid file to data table", "<GI,>DO"},
	{"grdblend", "core", "Blend several partially over-lapping grids into one larger grid", "<TI,GGO"},
	{"grdclip", "core", "Clip the range of grids", "<GI,GGO"},
	{"grdcontour", "core", "Make contour map using a grid", "<GI,CCI,-Xo"},
	{"grdconvert", "core", "Convert between different grid formats", "<GI,>GO"},
	{"grdcut", "core", "Extract subregion from a grid", "<GI,GGO"},
	{"grdedit", "core", "Modify header or content of a grid", "<GI,NDI,GGO"},
	{"grdfft", "core", "Do mathematical operations on grids in the wavenumber (or frequency) domain", "<GI,GGO"},
	{"grdfilter", "core", "Filter a grid in the space (or time) domain", "<GI,GGO"},
	{"grdgradient", "core", "Compute directional gradients from a grid", "<GI,GGO,SGo"},
	{"grdhisteq", "core", "Perform histogram equalization for a grid", "<GI,GGO,DTo"},
	{"grdimage", "core", "Project grids or images and plot them on maps", "<GI,CCI,IGI,-Xo"},
	{"grdinfo", "core", "Extract information from grids", "<GI,>TO"},
	{"grdlandmask", "core", "Create a \"wet-dry\" mask grid from shoreline data base", "-XI,GGO"},
	{"grdmask", "core", "Create mask grid from polygons or point coverage", "<DI,GGO"},
	{"grdmath", "core", "Reverse Polish Notation (RPN) calculator for grids (element by element)", "<GI,>GO"},
	{"grdpaste", "core", "Join two grids along their common edge", "<GI,GGO"},
	{"grdproject", "core", "Forward and inverse map transformation of grids", "<GI,GGO"},
	{"grdraster", "core", "Extract subregion from a binary raster and save as a GMT grid", "GGO,TDo"},
	{"grdsample", "core", "Resample a grid onto a new lattice", "<GI,GGO"},
	{"grdtrack", "core", "Sample grids at specified (x,y) locations", "<DI,DDo,GGI,>DO"},
	{"grdtrend", "core", "Fit trend surface to grids and compute residuals", "<GI,DGo,TGo,WGo"},
	{"grdvector", "core", "Plot vector field from two component grids", "<GI,CCI,-Xo"},
	{"grdview", "core", "Create 3-D perspective image or surface mesh from a grid", "<GI,CCI,GGI,IGI,-Xo"},
	{"grdvolume", "core", "Calculate grid volume and area constrained by a contour", "<GI,>DO"},
	{"greenspline", "core", "Interpolate using Green's functions for splines in 1-3 dimensions", "<DI,ADI,NDI,TGI,CDo,GGO"},
	{"kml2gmt", "core", "Extract GMT table data from Google Earth KML files", "<TI,>DO"},
	{"makecpt", "core", "Make GMT color palette tables", ">CO"},
	{"mapproject", "core", "Do forward and inverse map transformations, datum conversions and geodesy", "<DI,LDI,>DO"},
	{"nearneighbor", "core", "Grid table data using a \"Nearest neighbor\" algorithm", "<DI,GGO"},
	{"project", "core", "Project table data onto lines or great circles, generate tracks, or translate coordinates", "<DI,>DO"},
	{"psbasemap", "core", "Plot base maps and frames", "<-XI,-Xo"},
	{"psclip", "core", "Initialize or terminate polygonal clip paths", "<DI,-Xo"},
	{"pscoast", "core", "Plot continents, countries, shorelines, rivers, and borders on maps", "-XI,-Xo,>Do"},
	{"pscontour", "core", "Contour table data by direct triangulation", "<DI,CCI,QDI,-Xo"},
	{"psconvert", "core", "Convert [E]PS file(s) to other formats using GhostScript", "--O"},
	{"pshistogram", "core", "Calculate and plot histograms", "<DI,-Xo,>Do"},
	{"psimage", "core", "Place images or EPS files on maps", "<II,-Xo"},
	{"pslegend", "core", "Plot legends on maps", "<TI,-Xo"},
	{"psmask", "core", "Use data tables to clip or mask map areas with no coverage", "<DI,-Xo"},
	{"psrose", "core", "Plot a polar histogram (rose, sector, windrose diagrams)", "<DI,-Xo"},
	{"psscale", "core", "Plot a gray-scale or color-scale on maps", "-XI,CCI,-Xo"},
	{"pstext", "core", "Plot or typeset text on maps", "<TI,-Xo"},
	{"pswiggle", "core", "Plot z = f(x,y) anomalies along tracks", "<DI,-Xo"},
	{"psxyz", "core", "Plot lines, polygons, and symbols in 3-D", "<DI,CCI,-Xo"},
	{"psxy", "core", "Plot lines, polygons, and symbols on maps", "<DI,CCI,-Xo"},
	{"read", "core", "Read GMT objects into external API", "<?I,>?O"},
	{"sample1d", "core", "Resample 1-D table data using splines", "<DI,NDI,>DO"},
	{"spectrum1d", "core", "Compute auto- [and cross-] spectra from one [or two] timeseries", "<DI,>DO"},
	{"sph2grd", "core", "Compute grid from spherical harmonic coefficients", "<DI,GGO"},
	{"sphdistance", "core", "Make grid of distances to nearest points on a sphere", "<DI,NDI,QDI,GGO"},
	{"sphinterpolate", "core", "Spherical gridding in tension of data on a sphere", "<DI,GGO"},
	{"sphtriangulate", "core", "Delaunay or Voronoi construction of spherical lon,lat data", "<DI,GGO,NDo"},
	{"splitxyz", "core", "Split xyz[dh] data tables into individual segments", "<DI,>DO"},
	{"surface", "core", "Grid table data using adjustable tension continuous curvature splines", "<DI,DDI,GGO"},
	{"trend1d", "core", "Fit a [weighted] [robust] polynomial [or Fourier] model for y = f(x) to xy[w] data", "<DI,>DO"},
	{"trend2d", "core", "Fit a [weighted] [robust] polynomial for z = f(x,y) to xyz[w] data", "<DI,>DO"},
	{"triangulate", "core", "Do optimal (Delaunay) triangulation and gridding of Cartesian table data", "<DI,>DO,GGo"},
	{"write", "core", "Write GMT objects from external API", "<?I,>?O"},
	{"xyz2grd", "core", "Convert data table to a grid file", "<DI,SDo,GGO"},
	{NULL, NULL, NULL, NULL} /* last element == NULL detects end of array */
#else
	{"blockmean", "core", "Block average (x,y,z) data tables by L2 norm", "<DI,>DO", &GMT_blockmean},
	{"blockmedian", "core", "Block average (x,y,z) data tables by L1 norm (spatial median)", "<DI,>DO", &GMT_blockmedian},
	{"blockmode", "core", "Block average (x,y,z) data tables by mode estimation", "<DI,>DO", &GMT_blockmode},
	{"filter1d", "core", "Do time domain filtering of 1-D data tables", "<DI,>DO", &GMT_filter1d},
	{"fitcircle", "core", "Find mean position and best-fitting great- or small-circle to points on sphere", "<DI,>TO", &GMT_fitcircle},
	{"gmt2kml", "core", "Convert GMT data tables to KML files for Google Earth", "<DI,>TO", &GMT_gmt2kml},
	{"gmtconnect", "core", "Connect individual lines whose end points match within tolerance", "<DI,>DO,CDo,LTo,QTo", &GMT_gmtconnect},
	{"gmtconvert", "core", "Convert, paste, or extract columns from data tables", "<DI,>DO", &GMT_gmtconvert},
	{"gmtdefaults", "core", "List current GMT default parameters", ">TO", &GMT_gmtdefaults},
	{"gmtget", "core", "Get individual GMT default parameters", ">TO", &GMT_gmtget},
	{"gmtinfo", "core", "Get information about data tables", "<DI,>DO", &GMT_gmtinfo},
	{"gmtlogo", "core", "Plot the GMT logo on maps", "", &GMT_gmtlogo},
	{"gmtmath", "core", "Reverse Polish Notation (RPN) calculator for data tables", "<DI,ADI,>DO", &GMT_gmtmath},
	{"gmtregress", "core", "Perform linear regression of 1-D data sets", "<DI,>DO", &GMT_gmtregress},
	{"gmtselect", "core", "Select data table subsets based on multiple spatial criteria", "<DI,CDi,FDi,LDi,>DO", &GMT_gmtselect},
	{"gmtset", "core", "Change individual GMT default parameters", "<TI", &GMT_gmtset},
	{"gmtsimplify", "core", "Line reduction using the Douglas-Peucker algorithm", "<DI,>DO", &GMT_gmtsimplify},
	{"gmtspatial", "core", "Do geospatial operations on lines and polygons", "<DI,DDI,NDI,TDI,>DO", &GMT_gmtspatial},
	{"gmtvector", "core", "Basic manipulation of Cartesian vectors", "<DI,ADI,>DO", &GMT_gmtvector},
	{"gmtwhich", "core", "Find full path to specified files", "<TI,>TO", &GMT_gmtwhich},
	{"grd2cpt", "core", "Make linear or histogram-equalized color palette table from grid", "<GI,>CO", &GMT_grd2cpt},
	{"grd2rgb", "core", "Write r/g/b grid files from a grid file, a raw RGB file, or SUN rasterfile", "<GI", &GMT_grd2rgb},
	{"grd2xyz", "core", "Convert grid file to data table", "<GI,>DO", &GMT_grd2xyz},
	{"grdblend", "core", "Blend several partially over-lapping grids into one larger grid", "<TI,GGO", &GMT_grdblend},
	{"grdclip", "core", "Clip the range of grids", "<GI,GGO", &GMT_grdclip},
	{"grdcontour", "core", "Make contour map using a grid", "<GI,CCI,-Xo", &GMT_grdcontour},
	{"grdconvert", "core", "Convert between different grid formats", "<GI,>GO", &GMT_grdconvert},
	{"grdcut", "core", "Extract subregion from a grid", "<GI,GGO", &GMT_grdcut},
	{"grdedit", "core", "Modify header or content of a grid", "<GI,NDI,GGO", &GMT_grdedit},
	{"grdfft", "core", "Do mathematical operations on grids in the wavenumber (or frequency) domain", "<GI,GGO", &GMT_grdfft},
	{"grdfilter", "core", "Filter a grid in the space (or time) domain", "<GI,GGO", &GMT_grdfilter},
	{"grdgradient", "core", "Compute directional gradients from a grid", "<GI,GGO,SGo", &GMT_grdgradient},
	{"grdhisteq", "core", "Perform histogram equalization for a grid", "<GI,GGO,DTo", &GMT_grdhisteq},
	{"grdimage", "core", "Project grids or images and plot them on maps", "<GI,CCI,IGI,-Xo", &GMT_grdimage},
	{"grdinfo", "core", "Extract information from grids", "<GI,>TO", &GMT_grdinfo},
	{"grdlandmask", "core", "Create a \"wet-dry\" mask grid from shoreline data base", "-XI,GGO", &GMT_grdlandmask},
	{"grdmask", "core", "Create mask grid from polygons or point coverage", "<DI,GGO", &GMT_grdmask},
	{"grdmath", "core", "Reverse Polish Notation (RPN) calculator for grids (element by element)", "<GI,>GO", &GMT_grdmath},
	{"grdpaste", "core", "Join two grids along their common edge", "<GI,GGO", &GMT_grdpaste},
	{"grdproject", "core", "Forward and inverse map transformation of grids", "<GI,GGO", &GMT_grdproject},
	{"grdraster", "core", "Extract subregion from a binary raster and save as a GMT grid", "GGO,TDo", &GMT_grdraster},
	{"grdsample", "core", "Resample a grid onto a new lattice", "<GI,GGO", &GMT_grdsample},
	{"grdtrack", "core", "Sample grids at specified (x,y) locations", "<DI,DDo,GGI,>DO", &GMT_grdtrack},
	{"grdtrend", "core", "Fit trend surface to grids and compute residuals", "<GI,DGo,TGo,WGo", &GMT_grdtrend},
	{"grdvector", "core", "Plot vector field from two component grids", "<GI,CCI,-Xo", &GMT_grdvector},
	{"grdview", "core", "Create 3-D perspective image or surface mesh from a grid", "<GI,CCI,GGI,IGI,-Xo", &GMT_grdview},
	{"grdvolume", "core", "Calculate grid volume and area constrained by a contour", "<GI,>DO", &GMT_grdvolume},
	{"greenspline", "core", "Interpolate using Green's functions for splines in 1-3 dimensions", "<DI,ADI,NDI,TGI,CDo,GGO", &GMT_greenspline},
	{"kml2gmt", "core", "Extract GMT table data from Google Earth KML files", "<TI,>DO", &GMT_kml2gmt},
	{"makecpt", "core", "Make GMT color palette tables", ">CO", &GMT_makecpt},
	{"mapproject", "core", "Do forward and inverse map transformations, datum conversions and geodesy", "<DI,LDI,>DO", &GMT_mapproject},
	{"nearneighbor", "core", "Grid table data using a \"Nearest neighbor\" algorithm", "<DI,GGO", &GMT_nearneighbor},
	{"project", "core", "Project table data onto lines or great circles, generate tracks, or translate coordinates", "<DI,>DO", &GMT_project},
	{"psbasemap", "core", "Plot base maps and frames", "<-XI,-Xo", &GMT_psbasemap},
	{"psclip", "core", "Initialize or terminate polygonal clip paths", "<DI,-Xo", &GMT_psclip},
	{"pscoast", "core", "Plot continents, countries, shorelines, rivers, and borders on maps", "-XI,-Xo,>Do", &GMT_pscoast},
	{"pscontour", "core", "Contour table data by direct triangulation", "<DI,CCI,QDI,-Xo", &GMT_pscontour},
	{"psconvert", "core", "Convert [E]PS file(s) to other formats using GhostScript", "--O", &GMT_psconvert},
	{"pshistogram", "core", "Calculate and plot histograms", "<DI,-Xo,>Do", &GMT_pshistogram},
	{"psimage", "core", "Place images or EPS files on maps", "<II,-Xo", &GMT_psimage},
	{"pslegend", "core", "Plot legends on maps", "<TI,-Xo", &GMT_pslegend},
	{"psmask", "core", "Use data tables to clip or mask map areas with no coverage", "<DI,-Xo", &GMT_psmask},
	{"psrose", "core", "Plot a polar histogram (rose, sector, windrose diagrams)", "<DI,-Xo", &GMT_psrose},
	{"psscale", "core", "Plot a gray-scale or color-scale on maps", "-XI,CCI,-Xo", &GMT_psscale},
	{"pstext", "core", "Plot or typeset text on maps", "<TI,-Xo", &GMT_pstext},
	{"pswiggle", "core", "Plot z = f(x,y) anomalies along tracks", "<DI,-Xo", &GMT_pswiggle},
	{"psxyz", "core", "Plot lines, polygons, and symbols in 3-D", "<DI,CCI,-Xo", &GMT_psxyz},
	{"psxy", "core", "Plot lines, polygons, and symbols on maps", "<DI,CCI,-Xo", &GMT_psxy},
	{"read", "core", "Read GMT objects into external API", "<?I,>?O", &GMT_read},
	{"sample1d", "core", "Resample 1-D table data using splines", "<DI,NDI,>DO", &GMT_sample1d},
	{"spectrum1d", "core", "Compute auto- [and cross-] spectra from one [or two] timeseries", "<DI,>DO", &GMT_spectrum1d},
	{"sph2grd", "core", "Compute grid from spherical harmonic coefficients", "<DI,GGO", &GMT_sph2grd},
	{"sphdistance", "core", "Make grid of distances to nearest points on a sphere", "<DI,NDI,QDI,GGO", &GMT_sphdistance},
	{"sphinterpolate", "core", "Spherical gridding in tension of data on a sphere", "<DI,GGO", &GMT_sphinterpolate},
	{"sphtriangulate", "core", "Delaunay or Voronoi construction of spherical lon,lat data", "<DI,GGO,NDo", &GMT_sphtriangulate},
	{"splitxyz", "core", "Split xyz[dh] data tables into individual segments", "<DI,>DO", &GMT_splitxyz},
	{"surface", "core", "Grid table data using adjustable tension continuous curvature splines", "<DI,DDI,GGO", &GMT_surface},
	{"trend1d", "core", "Fit a [weighted] [robust] polynomial [or Fourier] model for y = f(x) to xy[w] data", "<DI,>DO", &GMT_trend1d},
	{"trend2d", "core", "Fit a [weighted] [robust] polynomial for z = f(x,y) to xyz[w] data", "<DI,>DO", &GMT_trend2d},
	{"triangulate", "core", "Do optimal (Delaunay) triangulation and gridding of Cartesian table data", "<DI,>DO,GGo", &GMT_triangulate},
	{"write", "core", "Write GMT objects from external API", "<?I,>?O", &GMT_write},
	{"xyz2grd", "core", "Convert data table to a grid file", "<DI,SDo,GGO", &GMT_xyz2grd},
	{NULL, NULL, NULL, NULL, NULL} /* last element == NULL detects end of array */
#endif
};

/* Pretty print all GMT core module names and their purposes */
void gmt_core_module_show_all (void *V_API) {
	unsigned int module_id = 0;
	char message[256];
	struct GMTAPI_CTRL *API = gmt_get_api_ptr (V_API);
	GMT_Message (V_API, GMT_TIME_NONE, "\n===  GMT core: The main modules of the Generic Mapping Tools  ===\n");
	while (g_core_module[module_id].name != NULL) {
		if (module_id == 0 || strcmp (g_core_module[module_id-1].component, g_core_module[module_id].component)) {
			/* Start of new supplemental group */
			sprintf (message, "\nModule name:     Purpose of %s module:\n", g_core_module[module_id].component);
			GMT_Message (V_API, GMT_TIME_NONE, message);
			GMT_Message (V_API, GMT_TIME_NONE, "----------------------------------------------------------------\n");
		}
		if (API->mode || (strcmp (g_core_module[module_id].name, "read") && strcmp (g_core_module[module_id].name, "write"))) {
			sprintf (message, "%-16s %s\n",
				g_core_module[module_id].name, g_core_module[module_id].purpose);
				GMT_Message (V_API, GMT_TIME_NONE, message);
		}
		++module_id;
	}
}

/* Lookup module id by name, return option keys pointer (for external API developers) */
const char * gmt_core_module_info (void *API, char *candidate) {
	int module_id = 0;

	/* Match actual_name against g_module[module_id].name */
	while ( g_core_module[module_id].name != NULL &&
			strcmp (candidate, g_core_module[module_id].name) )
		++module_id;

	/* Return Module keys or NULL */
	return (g_core_module[module_id].keys);
}
	
#ifndef BUILD_SHARED_LIBS
/* Lookup module id by name, return function pointer */
void * gmt_core_module_lookup (void *API, const char *candidate) {
	int module_id = 0;

	/* Match actual_name against g_module[module_id].name */
	while ( g_core_module[module_id].name != NULL &&
			strcmp (candidate, g_core_module[module_id].name) )
		++module_id;

	/* Return Module function or NULL */
	return (g_core_module[module_id].p_func);
}
#endif
