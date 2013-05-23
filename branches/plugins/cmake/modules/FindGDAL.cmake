#
# $Id$
#
# Locate gdal
#
# This module accepts the following environment variables:
#
#    GDAL_DIR or GDAL_ROOT - Specify the location of GDAL
#
# This module defines the following CMake variables:
#
#    GDAL_FOUND - True if libgdal is found
#    GDAL_LIBRARY - A variable pointing to the GDAL library
#    GDAL_INCLUDE_DIR - Where to find the headers

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See COPYING-CMAKE-SCRIPTS for more information.
#=============================================================================
# Note: this file is not an exact copy of the original file from Kitware.
#       It has been modified for the needs of GMT.

#
# $GDAL_DIR is an environment variable that would
# correspond to the ./configure --prefix=$GDAL_DIR
# used in building gdal.
#
# Created by Eric Wing. I'm not a gdal user, but OpenSceneGraph uses it
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real gdal users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems
# to happen).

# This makes the presumption that you are include gdal.h like
#
#include "gdal.h"

if (DEFINED GDAL_ROOT AND NOT GDAL_ROOT)
  set (GDAL_LIBRARY "" CACHE INTERNAL "")
  set (GDAL_INCLUDE_DIR "" CACHE INTERNAL "")
  return()
endif()

if (UNIX AND NOT GDAL_FOUND)
  # Use gdal-config to obtain the library version (this should hopefully
  # allow us to -lgdal1.x.y where x.y are correct version)
  # For some reason, libgdal development packages do not contain
  # libgdal.so...
  find_program (GDAL_CONFIG gdal-config
    HINTS
    ${GDAL_DIR}
    ${GDAL_ROOT}
    $ENV{GDAL_DIR}
    $ENV{GDAL_ROOT}
    PATH_SUFFIXES bin
    PATHS
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    )

  if (GDAL_CONFIG)
    execute_process (COMMAND ${GDAL_CONFIG} --cflags
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE GDAL_CONFIG_CFLAGS)
    if (GDAL_CONFIG_CFLAGS)
      string (REGEX MATCHALL "-I[^ ]+" _gdal_dashI ${GDAL_CONFIG_CFLAGS})
      string (REGEX REPLACE "-I" "" _gdal_includepath "${_gdal_dashI}")
      string (REGEX REPLACE "-I[^ ]+" "" _gdal_cflags_other ${GDAL_CONFIG_CFLAGS})
    endif (GDAL_CONFIG_CFLAGS)
    execute_process (COMMAND ${GDAL_CONFIG} --libs
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE GDAL_CONFIG_LIBS)
    if (GDAL_CONFIG_LIBS)
      string (REGEX MATCHALL "-l[^ ]+" _gdal_dashl ${GDAL_CONFIG_LIBS})
      string (REGEX REPLACE "-l" "" _gdal_lib "${_gdal_dashl}")
      string (REGEX MATCHALL "-L[^ ]+" _gdal_dashL ${GDAL_CONFIG_LIBS})
      string (REGEX REPLACE "-L" "" _gdal_libpath "${_gdal_dashL}")
    endif (GDAL_CONFIG_LIBS)
  endif (GDAL_CONFIG)
endif (UNIX AND NOT GDAL_FOUND)

find_path (GDAL_INCLUDE_DIR gdal.h
  HINTS
  ${_gdal_includepath}
  ${GDAL_DIR}
  ${GDAL_ROOT}
  $ENV{GDAL_DIR}
  $ENV{GDAL_ROOT}
  PATH_SUFFIXES
  include/gdal
  include/GDAL
  include
  PATHS
  ~/Library/Frameworks/gdal.framework/Headers
  /Library/Frameworks/gdal.framework/Headers
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
  )

find_library (GDAL_LIBRARY
  NAMES ${_gdal_lib} gdal gdal_i gdal1.5.0 gdal1.4.0 gdal1.3.2 GDAL
  HINTS
  ${GDAL_DIR}
  ${GDAL_ROOT}
  $ENV{GDAL_DIR}
  $ENV{GDAL_ROOT}
  ${_gdal_libpath}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks/gdal.framework
  /Library/Frameworks/gdal.framework
  /sw
  /opt/local
  /opt/csw
  /opt
  )

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GDAL DEFAULT_MSG GDAL_LIBRARY GDAL_INCLUDE_DIR)

set (GDAL_LIBRARIES ${GDAL_LIBRARY})
set (GDAL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})

# vim: textwidth=78 noexpandtab tabstop=2 softtabstop=2 shiftwidth=2
