/*--------------------------------------------------------------------
 *	$Id: gmt_error.h,v 1.2 2007-03-12 12:18:56 remko Exp $
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/

/*
 * Include file for GMT error codes
 *
 * Author:	Paul Wessel
 * Date:	7-MAR-2007
 * Version:	4.2
 */

#ifndef GMT_ERROR_H
#define GMT_ERROR_H

/* Grid i/o error codes */

#define GMT_NOERROR			0
#define GMT_GRDIO_UNKNOWN_FORMAT	-128
#define GMT_GRDIO_UNKNOWN_TYPE		-129
#define GMT_GRDIO_UNKNOWN_ID		-130
#define GMT_GRDIO_UNKNOWN_CODE		-131
#define GMT_GRDIO_PIPE_CODECHECK	-132
#define GMT_GRDIO_DOMAIN_VIOLATION	-133
#define GMT_GRDIO_OPEN_FAILED		-134
#define GMT_GRDIO_CREATE_FAILED		-135
#define GMT_GRDIO_READ_FAILED		-136
#define GMT_GRDIO_WRITE_FAILED		-137
#define GMT_GRDIO_STAT_FAILED		-138
#define GMT_GRDIO_SEEK_FAILED		-139
#define GMT_GRDIO_FILE_NOT_FOUND	-140
#define GMT_GRDIO_BAD_VAL		-141
#define GMT_GRDIO_BAD_XINC		-142
#define GMT_GRDIO_BAD_XRANGE		-143
#define GMT_GRDIO_BAD_XMATCH		-144
#define GMT_GRDIO_BAD_YINC		-145
#define GMT_GRDIO_BAD_YRANGE		-146
#define GMT_GRDIO_BAD_YMATCH		-147
#define GMT_GRDIO_BAD_IMG_LAT		-148
#define GMT_GRDIO_NO_2DVAR		-149
#define GMT_GRDIO_NO_VAR		-150
#define GMT_GRDIO_BAD_DIM		-151
#define GMT_GRDIO_NOT_NC		-152
#define GMT_GRDIO_NC_NO_PIPE		-153
#define GMT_GRDIO_NOT_RAS		-154
#define GMT_GRDIO_NOT_8BIT_RAS		-155
#define GMT_GRDIO_NOT_SURFER		-156
#define GMT_GRDIO_SURF7_UNSUPPORTED	-157
#define GMT_GRDIO_GRD98_XINC		-158
#define GMT_GRDIO_GRD98_YINC		-159
#define GMT_GRDIO_GRD98_BADMAGIC	-160
#define GMT_GRDIO_GRD98_BADLENGTH	-161
#define GMT_GRDIO_GRD98_COMPLEX		-162

/* Definition for an error trap */
#define GMT_err_trap(func_call) if ((err = (func_call)) != GMT_NOERROR) return(err)

EXTERN_MSC const char * GMT_strerror (int err);

#endif /* GMT_ERROR_H */
