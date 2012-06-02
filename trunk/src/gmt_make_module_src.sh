#!/bin/bash
#
# $Id$
#
# Copyright (c) ....
#
# Run this script after adding a new GMT module and updating the file
# gmt_moduleinfo.txt in order to generate gmt_module.h and gmt_module.c.
#

set -e

FILE_MODULEINFO=gmt_moduleinfo.txt
FILE_GMT_MODULE_C=gmt_module.c
FILE_GMT_MODULE_H=gmt_module.h

#
# Generate FILE_GMT_MODULE_H
#

cat << EOF > ${FILE_GMT_MODULE_H}
/* \$Id:\$
 *
 * Copyright (c) ....
 */

/* gmt_module.h declares the array that contains GMT module parameters such as
 * name and purpose strings, and function pointers to call the module functions.
 * Do not edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#pragma once
#ifndef _GMT_MODULE_H
#define _GMT_MODULE_H

#ifdef __cplusplus      /* Basic C++ support */
extern "C" {
#endif

/* CMake definitions: This must be first! */
#include "gmt_config.h"

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"

/* API mode */
enum Api_mode {
  _Api_mode_must_promote_to_int = -1,
  k_mode_gmt,
  k_mode_psl
};

struct GMTAPI_CTRL; /* Forward declaration of GMTAPI_CTRL */

/* name, purpose, Api_mode, and function pointer for each GMT module */
struct Gmt_moduleinfo {
  char *name;             /* Program name */
  char *component;        /* Component (core or supplement) */
  char *purpose;          /* Program purpose */
  enum Api_mode api_mode; /* Either k_mode_gmt or k_mode_psl*/
  /* gmt module function pointer: */
  int (*p_func)(struct GMTAPI_CTRL*, int, void*);
};

/* external array with program paramerters for all GMT modules */
extern struct Gmt_moduleinfo g_module[];

/* enumerator with same order and length as array g_module */
enum Gmt_module {
EOF

# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
  BEGIN {
    FS = "\t";
    first_record = 1;
  }
  /^[ \t]*#/ {
    next;
  }
  first_record {
    printf "  k_mod_%s = 0,\n", $1;
    first_record = 0;
    next;
  }
  {
    printf "  k_mod_%s,\n", $1;
  }' ${FILE_MODULEINFO} >> ${FILE_GMT_MODULE_H}

cat << EOF >> ${FILE_GMT_MODULE_H}
  k_mod_notfound
};

/* function prototypes of all GMT modules
 * EXTERN_MSC int GMT_* (struct GMTAPI_CTRL *api_ctrl, int mode, void *args); */
EOF

# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
  BEGIN {
    FS = "\t";
  }
  !/^[ \t]*#/ {
    printf "EXTERN_MSC int GMT_%s (struct GMTAPI_CTRL *api_ctrl, int mode, void *args);\n", $1;
  }' ${FILE_MODULEINFO} >> ${FILE_GMT_MODULE_H}

cat << EOF >> ${FILE_GMT_MODULE_H}

#ifdef __cplusplus
}
#endif

/* Pretty print all module names and their purposes */
void gmt_module_show_all();

/* Pretty print module names and purposes */
void gmt_module_show_name_and_purpose(struct Gmt_moduleinfo module);

/* Lookup module id by name */
enum Gmt_module gmt_module_lookup (char *candidate);

#endif /* !_GMT_MODULE_H */
EOF

#
# Generate FILE_GMT_MODULE_C
#

cat << EOF > ${FILE_GMT_MODULE_C}
/* \$Id:\$
 *
 * Copyright (c) ....
 */

/* gmt_module.c populates the external array of GMT module parameters such as name
 * and purpose strings, and function pointers to call the module functions.
 * Do not edit this file directly! Instead edit gmt_moduleinfo.txt and regenerate
 * this file with gmt_make_module_src.sh. */

#include <stdio.h>
#include <string.h>

#include "gmt_module.h"
#include "gmt_version.h"

/* sorted array with program paramerters for all GMT modules */
struct Gmt_moduleinfo g_module[] = {
EOF

# $1 = name, $2 = core/supplement, $3 = Api_mode, $4 = purpose
gawk '
  BEGIN {
    FS = "\t";
  }
  !/^[ \t]*#/ {
    printf "  {\"%s\", \"%s\", \"%s\", %s, &GMT_%s},\n", $1, $2, $4, $3, $1;
  }' ${FILE_MODULEINFO} >> ${FILE_GMT_MODULE_C}

cat << EOF >> ${FILE_GMT_MODULE_C}
  {NULL, NULL, NULL, -1, NULL} /* last element == NULL detects end of array */
};

/* Pretty print all module names and their purposes */
void gmt_module_show_all() {
	enum Gmt_module module_id = 0; /* Module ID */

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
void gmt_module_show_name_and_purpose(struct Gmt_moduleinfo module) {
	fprintf (stderr, "%s(%s) %s - %s\n\n",
			module.name,
			module.component,
			GMT_version(),
			module.purpose);
}

/* Lookup module id by name */
enum Gmt_module gmt_module_lookup (char *candidate) {
	struct Gmt_moduleinfo module;  /* Module parameters */
	enum Gmt_module module_id = 0; /* Module ID */

	/* Match candidate against g_module[module_id].name */
	while ( g_module[module_id].name != NULL &&
			strcmp (candidate, g_module[module_id].name ) )
		++module_id;

	/* Return matching Module ID or k_mod_notfound */
	return module_id;
}
EOF

exit 0

# vim: set ft=c:
