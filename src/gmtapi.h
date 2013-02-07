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
 * The single include file for users who wish to develop applications
 * that require building blocks from the GMT Application Program Interface
 * library (the GMT API), which also depends on the GMT Core library.
 *
 * Author: 	Paul Wessel
 * Date:	30-MAR-2010
 * Version:	5 API
 */

#ifndef _GMTAPI_H
#define _GMTAPI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Visual C++ only implements C90, which has no bool type. C99 added support
 * for bool via the <stdbool.h> header, but Visual C++ does not support this.
 */
#ifndef __bool_true_false_are_defined
#	if defined _MSC_VER
#		define bool _Bool
#		define true 1
#		define false 0
#		define __bool_true_false_are_defined 1
#	else
#		include <stdbool.h>
#	endif /* _MSC_VER */
#endif /* !__bool_true_false_are_defined */

/*
 * When an application links to a DLL in Windows, the symbols that
 * are imported have to be identified as such.
 */
#ifndef EXTERN_MSC
#	ifdef _WIN32
#		ifdef LIBRARY_EXPORTS
#			define EXTERN_MSC extern __declspec(dllexport)
#		else
#			define EXTERN_MSC extern __declspec(dllimport)
#		endif /* !LIBRARY_EXPORTS */
#	else /* !_WIN32 */
#		define EXTERN_MSC extern
#	endif /* _WIN32 */
#endif /* EXTERN_MSC */

#ifdef __cplusplus /* Basic C++ support */
extern "C" {
#endif

#include "gmtapi_define.h"
#include "gmtapi_resources.h"

/* API mode */
enum Api_mode {
	_Api_mode_must_promote_to_int = -1,
	k_mode_gmt, /* Need GMT but not PSL initialized */
	k_mode_psl  /* Need GMT and PSL initialized */
};

struct GMT_OPTION {	/* Structure for a single GMT command option */
	char option;			/* 1-char command line -<option> (e.g. D in -D) identifying the option (* if file) */
	char *arg;			/* If not NULL, contains the argument for this option */
	struct GMT_OPTION *next;	/* Pointer to next option in a linked list */
	struct GMT_OPTION *previous;	/* Pointer to previous option in a linked list */
};

/*=====================================================================================
 *	GMT API FUNCTION PROTOTYPES
 *=====================================================================================
 */

/* 17 Primary API functions */
EXTERN_MSC void * GMT_Create_Session	(char *tag, unsigned int mode);
EXTERN_MSC void * GMT_Create_Data	(void *C, unsigned int type, uint64_t par[]);
EXTERN_MSC void * GMT_Get_Data		(void *C, int object_ID, unsigned int mode, void *data);
EXTERN_MSC void * GMT_Read_Data		(void *C, unsigned int family, unsigned int method, unsigned int geometry, unsigned int mode, double wesn[], char *input, void *data);
EXTERN_MSC void * GMT_Retrieve_Data	(void *C, int object_ID);
EXTERN_MSC void * GMT_Get_Record	(void *C, unsigned int mode, int *retval);
EXTERN_MSC int GMT_Destroy_Session	(void *C);
EXTERN_MSC int GMT_Register_IO		(void *C, unsigned int family, unsigned int method, unsigned int geometry, unsigned int direction, double wesn[], void *resource);
EXTERN_MSC int GMT_Init_IO		(void *C, unsigned int family, unsigned int geometry, unsigned int direction, unsigned int mode, unsigned int n_args, void *args);
EXTERN_MSC int GMT_Begin_IO		(void *C, unsigned int family, unsigned int direction);
EXTERN_MSC int GMT_End_IO		(void *C, unsigned int direction, unsigned int mode);
EXTERN_MSC int GMT_Report_Error		(void *C, int error);
EXTERN_MSC int GMT_Put_Data		(void *C, int object_ID, unsigned int mode, void *data);
EXTERN_MSC int GMT_Write_Data		(void *C, unsigned int family, unsigned int method, unsigned int geometry, unsigned int mode, double wesn[], char *output, void *data);
EXTERN_MSC int GMT_Destroy_Data		(void *C, unsigned int mode, void *object);
EXTERN_MSC int GMT_Put_Record		(void *C, unsigned int mode, void *record);
EXTERN_MSC int GMT_Encode_ID		(void *C, char *string, int object_ID);

/* 12 secondary functions for argument and option parsing */

EXTERN_MSC struct GMT_OPTION * GMT_Create_Options	(void *C, int argc, void *in);
EXTERN_MSC struct GMT_OPTION * GMT_Prep_Options		(void *C, int mode, void *in);
EXTERN_MSC struct GMT_OPTION * GMT_Make_Option		(void *C, char option, char *arg);
EXTERN_MSC struct GMT_OPTION * GMT_Find_Option		(void *C, char option, struct GMT_OPTION *head);
EXTERN_MSC struct GMT_OPTION * GMT_Append_Option	(void *C, struct GMT_OPTION *current, struct GMT_OPTION *head);
EXTERN_MSC char ** GMT_Create_Args			(void *C, int *argc, struct GMT_OPTION *head);
EXTERN_MSC char * GMT_Create_Cmd			(void *C, struct GMT_OPTION *head);
EXTERN_MSC int GMT_Destroy_Options			(void *C, struct GMT_OPTION **head);
EXTERN_MSC int GMT_Destroy_Args				(void *C, int argc, char *argv[]);
EXTERN_MSC int GMT_Update_Option			(void *C, char option, char *arg, struct GMT_OPTION *head);
EXTERN_MSC int GMT_Delete_Option			(void *C, struct GMT_OPTION *current);
EXTERN_MSC int GMT_Parse_Common				(void *C, char *sorted, char *unsorted, struct GMT_OPTION *options);

/* Macro to test if filename is a special name indicating memory location */

#define GMT_File_Is_Memory(file) (file && !strncmp (file, "@GMTAPI@-", 9U))

#ifdef __cplusplus
}
#endif

#endif /* _GMTAPI_H */
