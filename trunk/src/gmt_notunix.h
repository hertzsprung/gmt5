/*--------------------------------------------------------------------
 *	$Id: gmt_notunix.h,v 1.34 2011-01-02 20:09:35 guru Exp $
 *
 *	Copyright (c) 1991-2011 by P. Wessel and W. H. F. Smith
 *	See LICENSE.TXT file for copying and redistribution conditions.
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
 * gmt_notunix.h contains definitions for constants, structures, and
 * other parameters parameters used by GMT that are not supported by
 * non-UNIX operating systems such as Microsoft Windows.  Note that
 * all of these entities are part of POSIX.  The contents of this file
 * is only activated if one of the system preprocessor flags are defined.
 * Currently considered non-UNIX systems include
 *
 *	flag    |	OS
 *	--------------------------------------------
 *	WIN32   | Microsoft Windows 9x, NT, 2000
 *	__EMX__ | IBM OS/2 with EMX support
 *	_WIN32  | Unix emulation under Windows, like
 *		| Cygwin or DJGPP.  WIN32 implies and
 *		| sets _WIN32 but the converse is not
 *		| true.
 *
 * Author:	Paul Wessel
 * Date:	09-NOV-1999
 * Version:	4.1.x
 *
 */

#ifndef _GMT_NOTUNIX_H
#define _GMT_NOTUNIX_H

/* A few general comments:
 * FLOCK is a pain. If cannot be used under EMX and _WIN32.
 * Also, users have problems with file locking because their 
 * NFS does not support it. Only those who are really sure should
 * activate -DFLOCK.
 */


/*--------------------------------------------------------------------
 *
 *	   W I N D O W S   1 9 9 5,  1 9 9 8,  2 0 0 0,  N T
 *
 *	 This section applies to Microsoft Windows 95, 98, NT
 *
 *--------------------------------------------------------------------*/

#ifdef WIN32	/* Start of Windows setup */

/* This section will override those in gmt_notposix.h which cannot
 * automatically be generated under Windows.
 */

/* Turn off some annoying "security" warnings in Vis Studio */

#pragma warning( disable : 4996 )

#define SET_IN_NOTUNIX	/* This forces the following not to be reset in gmt_notposix.h */

/* These functions are available under Windows with MSVC compilers */

#define HAVE_COPYSIGN 1
#define HAVE_LOG1P 0
#define HAVE_HYPOT 1
#define HAVE_ACOSH 0
#define HAVE_ASINH 0
#define HAVE_ATANH 0
#define HAVE_RINT 0
#define HAVE_IRINT 0
#define HAVE_ISNANF 0
#define HAVE_ISNAND 0
#define HAVE_ISNAN 1
#define HAVE_J0 1
#define HAVE_J1 1
#define HAVE_JN 1
#define HAVE_Y0 1
#define HAVE_Y1 1
#define HAVE_YN 1
#define HAVE_ERF 0
#define HAVE_ERFC 0
#define HAVE_STRDUP 1
#define HAVE_STRTOD 1
#ifdef __INTEL_COMPILER 
#define HAVE_SINCOS 1
#else
#define HAVE_SINCOS 0
#endif
#define HAVE_ALPHASINCOS 0
#define WORDS_BIGENDIAN 0

/* Several math functions exist but the names have a leading underscore */

#define copysign(x,y) _copysign(x,y)
#define hypot(x,y) _hypot(x,y)
#define isnan(x) _isnan(x)
#define j0(x) _j0(x)
#define j1(x) _j1(x)
#define jn(n,x) _jn(n,x)
#define y0(x) _y0(x)
#define y1(x) _y1(x)
#define yn(n,x) _yn(n,x)
#define strdup(s) _strdup(s)
#define STAT _stat

typedef int mode_t;		/* mode_t not defined under Windows; assumed a signed 4-byte integer */

/* WIN32 versus _WIN32:
 *
 * In GMT, the WIN32 flag is predefined by the MicroSoft C compiler.
 * If set, we assume we are in a non-posix environment and must make
 * up the missing functions with homespun code.
 * WIN32 will set _WIN32 but the converse is not true.
 *
 * _WIN32 is set whenever we are compiling GMT on a PC not running
 * a Unix flavor.  This is true when GMT is to be installed under
 * Cygwin32.  _WIN32, when set, causes the directory delimiter to
 * be set to \ instead of /, and also attempts to deal with the fact
 * that DOS file systems have both TEXT and BINARY file modes.
 */

#ifndef _WIN32
#define _WIN32
#endif

#define DIR_DELIM '\\'		/* Backslash as directory delimiter */
#define PATH_DELIM ';'		/* Win uses ;, Unix uses : */
#define PATH_SEPARATOR ";"	/* Win uses ;, Unix uses : */

#include <io.h>
#include <direct.h>

/* GMT normally gets these macros from unistd.h */

#define R_OK 04
#define W_OK 02
#define X_OK 01
#define F_OK 00

/* This structure is normally taken from pwd.h */

struct passwd {
	char	*pw_name;
	int	pw_uid;
	int	pw_gid;
	char	*pw_dir;
	char	*pw_shell;
};

/* These two functions prototypes are normally in pwd.h & unistd.h;
 * Here, they are defined as dummies at the bottom of gmt_init.c
 * since there are no equivalents under Windows. */

EXTERN_MSC struct passwd *getpwuid (const int uid);
EXTERN_MSC int getuid (void);

/* getcwd is usually in unistd.h; we use a macro here
 * since the same function under WIN32 is prefixed with _;
 * it is defined in direct.h. */

#ifndef getcwd
#define getcwd(path, len) _getcwd(path, len)
#endif

/* access is usually in unistd.h; we use a macro here
 * since the same function under WIN32 is prefixed with _
 * and defined in io.h */

#define access(path, mode) _access(path, mode)

/* mkdir is usually in sys/stat.h; we use a macro here
 * since the same function under WIN32 is prefixed with _
 * and furthermore does not pass the mode argument;
 * it is defined in direct.h */

#define mkdir(path,mode) _mkdir(path)

/* fileno is usually in stdio.h; we use a macro here
 * since the same function under WIN32 is prefixed with _
 * and defined in stdio.h */

#define fileno(stream) _fileno(stream)

/* setmode is usually in unistd.h; we use a macro here
 * since the same function under WIN32 is prefixed with _
 * and defined in io.h */

#define setmode(fd,mode) _setmode(fd,mode)

EXTERN_MSC void GMT_setmode (int i_or_o);

#endif		/* End of Windows setup */

/*--------------------------------------------------------------------
 *
 *	 		  O S / 2
 *
 *	 This section applies to OS/2 with EMX support
 *
 *--------------------------------------------------------------------*/
 
#ifdef __EMX__	/* Start of OS/2 with EMX support */
/*
 *   Definitions to aid the porting of GMT to OS/2
 *   Most of the porting is taken care of by configure.
 *
 * Author:	Allen Cogbill, Los Alamos National Laboratory
 * Date:	09-NOV-1999
 */
 

#define SET_IN_NOTUNIX	/* This forces the following not to be reset in gmt_notposix.h */

/* This applies specifically to O/S2 with EMX and Sun Free math library */

#define HAVE_COPYSIGN 1
#define HAVE_LOG1P 1
#define HAVE_HYPOT 1
#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_RINT 1
#define HAVE_IRINT 0
#define HAVE_ISNANF 1
#define HAVE_ISNAND 1
#define HAVE_ISNAN 1
#define HAVE_J0 1
#define HAVE_J1 1
#define HAVE_JN 1
#define HAVE_Y0 1
#define HAVE_Y1 1
#define HAVE_YN 1
#define HAVE_ERF 1
#define HAVE_ERFC 1
#define HAVE_STRDUP 1
#define HAVE_STRTOD 1
#define HAVE_SINCOS 0
#define HAVE_ALPHASINCOS 0
#define WORDS_BIGENDIAN 0

#include <io.h>

#undef FLOCK		/* Do not support file locking */
#define SET_IO_MODE	/* Need to force binary i/o upon request */
#define NO_FCNTL	/* fcntl.h does not exist here */
#define STAT _stat

EXTERN_MSC void GMT_setmode (int i_or_o);

#endif		/* End of OS/2 with EMX support */


/*--------------------------------------------------------------------
 *
 *	 		  NON-UNIX
 *
 *	 This section applies to WIN32, Cygwin, and possibly DJGPP
 *
 *--------------------------------------------------------------------*/
 
#ifdef _WIN32	/* Start of NON-UNIX */

#undef FLOCK		/* Do not support file locking */
#define SET_IO_MODE	/* Need to force binary i/o upon request */

EXTERN_MSC void GMT_setmode (int i_or_o);

#endif		/* End of NON-UNIX */

/*===================================================================
 *		      U N I X   C L E AN - U P
 *===================================================================*/
 
/* Set a few Default Unix settings if they did not get set above */


#ifndef DIR_DELIM
#define DIR_DELIM '/'
#endif

#ifndef PATH_DELIM
#define PATH_DELIM ':'		/* Win uses ;, Unix uses : */
#endif
#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR ":"	/* Win uses ;, Unix uses : */
#endif

#ifndef NO_FCNTL
#include <fcntl.h>
#endif

#ifndef STAT
#define STAT stat
#endif

#endif /* _GMT_NOTUNIX_H */
