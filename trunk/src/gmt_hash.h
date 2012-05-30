/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 * gmt_hash.h contains definition of the structure used for hashing.
 *
 * Author:	Paul Wessel
 * Date:	01-OCT-2009
 * Version:	5 API
 */

#ifndef _GMT_HASH_H
#define _GMT_HASH_H

/*--------------------------------------------------------------------
 *			GMT HASH STRUCTURE DEFINITION
 *--------------------------------------------------------------------*/

struct GMT_HASH {	/* Used to relate keywords to gmt.conf entry */
	struct GMT_HASH *next;
	unsigned int id;
	char *key;
};

#endif  /* _GMT_HASH_H */
