/* $Id$
 *
 * Copyright (c) 2012-2013
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_suppl_module.c populates the external array of GMT suppl
 * module parameters such as name, group and purpose strings.
 * This file also contains the following convenience function to
 * display all module purposes:
 *
 *   void gmt_suppl_module_show_all (struct GMTAPI_CTRL *API);
 *
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt
 * and regenerate this file with gmt_make_module_src.sh suppl */

#include "gmt_dev.h"

/* Sorted array with information for all GMT suppl modules */

struct Gmt_moduleinfo g_suppl_module[] = {
	{"gshhg", "gshhg", "Extract data tables from binary GSHHS or WDBII data files"},
	{"img2grd", "img", "Extract a subset from an img file in Mercator or Geographic format"},
	{"pscoupe", "meca", "Plot cross-sections of focal mechanisms"},
	{"psmeca", "meca", "Plot focal mechanisms on maps"},
	{"pspolar", "meca", "Plot polarities on the inferior focal half-sphere on maps"},
	{"psvelo", "meca", "Plot velocity vectors, crosses, and wedges on maps"},
	{"mgd77convert", "mgd77", "Convert MGD77 data to other file formats"},
	{"mgd77info", "mgd77", "Extract information about MGD77 files"},
	{"mgd77list", "mgd77", "Extract data from MGD77 files"},
	{"mgd77magref", "mgd77", "Evaluate the IGRF or CM4 magnetic field models"},
	{"mgd77manage", "mgd77", "Manage the content of MGD77+ files"},
	{"mgd77path", "mgd77", "Return paths to MGD77 cruises and directories"},
	{"mgd77sniffer", "mgd77", "Along-track quality control of MGD77 cruises"},
	{"mgd77track", "mgd77", "Plot track-line map of MGD77 cruises"},
	{"dimfilter", "misc", "Directional filtering of grids in the space domain"},
	{"gmtgravmag3d", "potential", "Compute the gravity/magnetic anomaly of a body by the method of Okabe"},
	{"gravfft", "potential", "Compute gravitational attraction of 3-D surfaces and a little more (ATTENTION z positive up)"},
	{"grdgravmag3d", "potential", "Computes the gravity effect of one (or two) grids by the method of Okabe"},
	{"grdredpol", "potential", "Compute the Continuous Reduction To the Pole, AKA differential RTP"},
	{"grdseamount", "potential", "Compute synthetic seamount (Gaussian or cone, circular or elliptical) bathymetry"},
	{"pssegyz", "segy", "Plot a SEGY file on a map in 3-D"},
	{"pssegy", "segy", "Plot a SEGY file on a map"},
	{"segy2grd", "segy", "Converting SEGY data to a GMT grid"},
	{"backtracker", "spotter", "Generate forward and backward flowlines and hotspot tracks"},
	{"grdpmodeler", "spotter", "Evaluate a plate model on a geographic grid"},
	{"grdrotater", "spotter", "Finite rotation reconstruction of geographic grid"},
	{"grdspotter", "spotter", "Create CVA image from a gravity or topography grid"},
	{"hotspotter", "spotter", "Create CVA image from seamount locations"},
	{"originator", "spotter", "Associate seamounts with nearest hotspot point sources"},
	{"rotconverter", "spotter", "Manipulate total reconstruction and stage rotations"},
	{"x2sys_binlist", "x2sys", "Create bin index listing from track data files"},
	{"x2sys_cross", "x2sys", "Calculate crossovers between track data files"},
	{"x2sys_datalist", "x2sys", "Extract content of track data files"},
	{"x2sys_get", "x2sys", "Get track listing from track index database"},
	{"x2sys_init", "x2sys", "Initialize a new x2sys track database"},
	{"x2sys_list", "x2sys", "Extract subset from crossover data base"},
	{"x2sys_merge", "x2sys", "Merge an updated COEs table (smaller) into the main table (bigger)"},
	{"x2sys_put", "x2sys", "Update track index database from track bin file"},
	{"x2sys_report", "x2sys", "Report statistics from crossover data base"},
	{"x2sys_solve", "x2sys", "Determine least-squares systematic correction from crossovers"},
	{NULL, NULL, NULL} /* last element == NULL detects end of array */
};

/* Pretty print all GMT suppl module names and their purposes */
void gmt_suppl_module_show_all (void *API) {
	unsigned int module_id = 0;
	char message[256];

	GMT_Message (API, GMT_TIME_NONE, "\n=== " "GMT suppl: The official supplements to the Generic Mapping Tools" " ===\n");
	while (g_suppl_module[module_id].name != NULL) {
		if (module_id == 0 || strcmp (g_suppl_module[module_id-1].component, g_suppl_module[module_id].component)) {
			/* Start of new supplemental group */
			sprintf (message, "\nModule name:     Purpose of %s module:\n", g_suppl_module[module_id].component);
			GMT_Message (API, GMT_TIME_NONE, message);
			GMT_Message (API, GMT_TIME_NONE, "----------------------------------------------------------------\n");
		}
		sprintf (message, "%-16s %s\n",
			g_suppl_module[module_id].name, g_suppl_module[module_id].purpose);
		GMT_Message (API, GMT_TIME_NONE, message);
		++module_id;
	}
}
