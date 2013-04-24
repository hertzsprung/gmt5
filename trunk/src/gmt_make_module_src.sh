#!/bin/bash
#
# $Id$
#
# Copyright (c) 2012-2013
# by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
# See LICENSE.TXT file for copying and redistribution conditions.
#
# Run this script after adding a new GMT module and updating the file
# gmt_moduleinfo.txt in order to generate the three files:
# gmt_module.h, gmt_module_private.h and gmt_module.c.
#
# Note: gmt_module.h, gmt_module_private.h and gmt_module.c are in svn
#

set -e

FILE_MODULEINFO=gmt_moduleinfo.txt
FILE_GMT_MODULE_C=gmt_module.c
FILE_GMT_MODULE_H=gmt_module.h
FILE_GMT_MODULE_PRIVATE_H=gmt_module_private.h
FILE_GMT_MODULE_TROFF=explain_gmt_modules.txt
COPY_YEAR=$(date +%Y)

#
# Generate FILE_GMT_MODULE_H
#

cat << EOF > ${FILE_GMT_MODULE_H}
/* \$Id\$
 *
 * Copyright (c) 2012-${COPY_YEAR}
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_module.h declares the function pointers to call the module functions.
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#pragma once
#ifndef _GMT_MODULE_H
#define _GMT_MODULE_H

#ifdef __cplusplus /* Basic C++ support */
extern "C" {
#endif

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"

EOF

cat << EOF > ${FILE_GMT_MODULE_PRIVATE_H}
/* \$Id\$
 *
 * Copyright (c) 2012-${COPY_YEAR}
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

EOF
# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
	BEGIN {
		FS = "\t";
	}
	!/^[ \t]*#/ {
		printf "EXTERN_MSC int GMT_%s (void *api_ctrl, int mode, void *args);\n", $1;
	}' ${FILE_MODULEINFO} >> ${FILE_GMT_MODULE_PRIVATE_H}

cat << EOF >> ${FILE_GMT_MODULE_H}
/* These enums can be used as module IDs in GMT_Call_Module. Alternatively,
 * obtain the corrensponding ID from the module name via GMT_Get_Module.
 */

enum GMT_MODULE_ID {
	GMT_ID_NONE = -1,
EOF

gawk '
	BEGIN {
		FS = "\t";
		first_record = 1;
	}
	/^[ \t]*#/ {
		next;
	}
	first_record {
		printf "\tGMT_ID_%s = 0,\n", $1;
		first_record = 0;
		next;
	}
	{
		printf "\tGMT_ID_%s,\n", $1;
	}' ${FILE_MODULEINFO} | tr 'a-z' 'A-Z' >> ${FILE_GMT_MODULE_H}

cat << EOF >> ${FILE_GMT_MODULE_H}
	GMT_N_MODULES
};

#ifdef __cplusplus
}
#endif

#endif /* !_GMT_MODULE_H */
EOF

cat << EOF >> ${FILE_GMT_MODULE_PRIVATE_H}

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
EOF

#
# Generate FILE_GMT_MODULE_C
#

cat << EOF > ${FILE_GMT_MODULE_C}
/* \$Id\$
 *
 * Copyright (c) 2012-${COPY_YEAR}
 * by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe
 * See LICENSE.TXT file for copying and redistribution conditions.
 */

/* gmt_module.c populates the external array of GMT module parameters such as name
 * and purpose strings, and function pointers to call the module functions.
 * This file also contains the following convenience functions to access the module
 * parameter array:
 *   void gmt_module_show_all(); - Pretty print all module names and their
 *           purposes
 *   void gmt_module_show_name_and_purpose(enum GMT_MODULE_ID module); - Pretty
 *           print module names and purposes
 *   enum GMT_MODULE_ID gmt_module_lookup (const char *candidate); - Lookup module id by
 *           name
 *   const char *gmt_module_name (struct GMT_CTRL *gmt_ctrl); - Get module name
 *
 * DO NOT edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#include <stdio.h>
#include <string.h>

#include "gmt_dev.h"

/* sorted array with program paramerters for all GMT modules */
struct Gmt_moduleinfo g_module[] = {
EOF

# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
	BEGIN {
		FS = "\t";
	}
	!/^[ \t]*#/ {
		printf "\t{\"%s\", \"%s\", \"%s\", &GMT_%s},\n", $1, $2, $3, $1;
	}' ${FILE_MODULEINFO} >> ${FILE_GMT_MODULE_C}

cat << EOF >> ${FILE_GMT_MODULE_C}
	{NULL, NULL, NULL, NULL} /* last element == NULL detects end of array */
};

/* Pretty print all module names and their purposes */
void gmt_module_show_all() {
	enum GMT_MODULE_ID module_id = 0; /* Module ID */
	char module_name_comp[GMT_TEXT_LEN64];

	fprintf (stderr, "Program                Purpose of Program\n");
	while (g_module[module_id].name != NULL) {
		snprintf (module_name_comp, GMT_TEXT_LEN64, "%s(%s)",
				g_module[module_id].name, g_module[module_id].component);
		fprintf (stderr, "%-22s %s\n",
				module_name_comp, g_module[module_id].purpose);
		++module_id;
	}
}

/* Pretty print module names and purposes */
void gmt_module_show_name_and_purpose(enum GMT_MODULE_ID module_id) {
	assert (module_id != GMT_ID_NONE);
	fprintf (stderr, "%s(%s) %s - %s\n\n",
			g_module[module_id].name,
			g_module[module_id].component,
			GMT_version(),
			g_module[module_id].purpose);
}

/* Lookup module id by name */
enum GMT_MODULE_ID gmt_module_lookup (const char *candidate) {
	enum GMT_MODULE_ID module_id = 0; /* Module ID */

	/* Match candidate against g_module[module_id].name */
	while ( g_module[module_id].name != NULL &&
			strcmp (candidate, g_module[module_id].name) )
		++module_id;

	/* Return matching Module ID or GMT_ID_NONE */
	return (module_id == GMT_N_MODULES) ? GMT_ID_NONE : module_id;
}

/* Get module name */
const char *gmt_module_name (struct GMT_CTRL *gmt_ctrl) {
	static const char no_module[] = "core"; /* when called before GMT_begin_module */
	const char *module_name;
	module_name = gmt_ctrl->init.module_id == GMT_ID_NONE ?
			gmt_ctrl->init.module_name : g_module[gmt_ctrl->init.module_id].name;
	if (module_name == NULL)
		/* when called before GMT_begin_module or after GMT_end_module */
		return no_module;
	return module_name;
}
EOF

#
# Generate FILE_GMT_MODULE_TROFF
#

# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
	BEGIN {
		FS = "\t";
	}
	!/^[ \t]*#/ {
		name = $1;
		suppl = $2;
		purpose = $4;
		if (suppl != "core") # remove "core" from list
			a_suppl_names[suppl] = suppl; # array of suppl names (once)
		a_mod_names[name] = name; # array of module names (ind = name)
		a_suppl[name] = suppl; # array of suppl names (multiple times, ind = name)
		a_purpose[name] = purpose; # array of purposes
	}
	END {
		n_mod_names = asort(a_mod_names); # sort array of module names
		n_suppl_names = asort(a_suppl_names); # sort array of suppl names
		printf ".TS\nl l .\n" # begin table
		a_suppl_names[0] = "core"; # insert "core" first
		for (ind_suppl = 0; ind_suppl <= n_suppl_names; ++ind_suppl) {
			# for each suppl name:
			if (ind_suppl > 0)
				printf "\n\tSupplement \\fI%s\\fP:\n", a_suppl_names[ind_suppl];
			for (ind_mod_name = 1; ind_mod_name <= n_mod_names; ++ind_mod_name) {
				mod_name = a_mod_names[ind_mod_name];
				if (a_suppl[mod_name] == a_suppl_names[ind_suppl])
					printf "\\fB%s\\fP\t%s\n", mod_name, a_purpose[mod_name];
			}
		}
		printf ".TE\n" # end table
	}' ${FILE_MODULEINFO} > ${FILE_GMT_MODULE_TROFF}

exit 0

# vim: set ft=c:
