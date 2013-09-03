@ECHO OFF
REM ----------------------------------------------------
REM
REM	$Id$
REM
REM
REM	Copyright (c) 1991-2010 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
REM	See LICENSE.TXT file for copying and redistribution conditions.
REM
REM	This program is free software; you can redistribute it and/or modify
REM	it under the terms of the GNU General Public License as published by
REM	the Free Software Foundation; version 2 or any later version.
REM
REM	This program is distributed in the hope that it will be useful,
REM	but WITHOUT ANY WARRANTY; without even the implied warranty of
REM	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM	GNU General Public License for more details.
REM
REM	Contact info: gmt.soest.hawaii.edu
REM --------------------------------------------------------------------
REM This extremely lame DOS batch file will compile
REM the GMT 5 suite of programs under WIN32|64 using
REM Microsoft Visual or Intel C/C++ tools.  It will build GMT and supplements
REM using DLL libraries. 
REM
REM Author: Joaquim Luis & Paul Wessel, 06-OCT-2010
REM ----------------------------------------------------
REM
REM How to build GMT executables under Windows:
REM Actually, before using this batch files successfully, several files need to be generated
REM from the GMT tree. On *nix this is done by the configure script, but on Windows we
REM currently have no way to do the same job. So in practice, to build the Windows version
REM it is necessary to build it first under Cygwin (or operate on a provided source tree).

REM
REM STEP a: Specify your compiler (Tested with MS CL and Intel ICL)
SET CC=CL

REM
REM STEP b: Specify the "Bitage" and if building normal or debug version
REM         Set DEBUG to "yes" or "no" and BITS = 32 or 64 (no quotes)
REM	    NOTE: The value set here for BITS will be the default but it
REM		  can be overriden by a third input arg.
SET DEBUG="yes"
SET BITS=64

REM
REM If two input args, they must contain NETCDF & GDAL base dirs. 
REM Otherwise this batch uses the default paths below.
REM A third input arg will be used to set the "Bitage" (default was set above).
REM An optional fourth input sets the path for the PCRE dir. This directory must than
REM two sub-dirs named 'lib' and 'include'. Still, for building with PCRE support the
REM STEP h must have been set to "yes" (Default)

IF  "%2%" == "" (
rem SET NETCDF_DIR=C:\progs_cygw\netcdf-3.6.3
SET NETCDF_DIR=C:\programs\compa_libs\netcdf-4.1.2b\compileds\VC10_%BITS%
SET GDAL_DIR=C:\programs\GDALtrunk\gdal\compileds\VC10_%BITS%\
) ELSE (
SET NETCDF_DIR=%1%
SET GDAL_DIR=%2%
)
IF "%3%" == "32" (
SET BITS=%3%
) ELSE (
IF "%3%" == "64" (
SET BITS=%3%)
)
IF  "%4%" == "" (
SET PCRE_DIR=C:\programs\compa_libs\pcre-8.12\compileds\VC10_%BITS%\
) ELSE (
SET PCRE_DIR=%4%
)

REM
REM STEP c: Install netcdf 3.6.3 as provided by the GMT site
REM         and adjust below for your own paths.
REM	    Note: If you are using other netcdf lib_netcdf may have a different name
REM	    NETCDF	Top dir of the netcdf installation tree
REM	    lib_netcdf	Name of the netCDF .lib library
REM
REM
SET LIB=%LIB%;%NETCDF_DIR%\LIB
rem SET lib_netcdf=libnetcdf_w%BITS%.lib
SET lib_netcdf=libnetcdf.lib

REM STEP  : Set the environment needed by GMT.  These are
REM	    GMTHOME	Top dir of the GMT installation tree, e.g., C:\GMT
REM	    GMT5_SHAREDIR	Place to read GMT run-time files.  If not
REM				set we use %GMTHOME%\share
REM	    PATH	Add the path to the netCDF DLL library netcdf.dll
REM	    PATH	Add the path to the GMT executables (%GMTHOME%\bin)

REM STEP d: Check/edit definitions of BINDIR, LIBDIR, and GMT_SHARE_PATH.
REM	    Make sure BINDIR below points to a valid directory
REM	    where you want executables to be installed. Either
REM	    edit BINDIR or create the ..\bin directory.
REM	    Same goes for LIBDIR where GMT libraries will be kept.
REM	    GMT_SHARE_PATH is where GMT expects to find the shared data.
REM	    It is ONLY used if the user does not set %GMT_SHAREDIR%.
REM
IF %BITS%==64 (
SET BINDIR=..\WIN64\bin
SET LIBDIR=..\WIN64\lib
SET INCDIR=..\WIN64\include\gmt
) ELSE (
SET BINDIR=..\WIN32\bin
SET LIBDIR=..\WIN32\lib
SET INCDIR=..\WIN32\include\gmt
)
SET GMT_SHARE_PATH="\"C:\\programs\\GMT5\\share\""

REM STEP e: To optionally link against the GDAL library you must set GDAL to "yes"
REM	    The right path to GDAL_DIR must have been set already (top) or sent as input
SET GDAL="yes"
SET GDAL_INC=
SET GDAL_LIB=
SET USE_GDAL=
IF  %GDAL%=="yes" SET GDAL_INC="/I%GDAL_DIR%\include" 
IF  %GDAL%=="yes" SET GDAL_LIB=%GDAL_DIR%\lib\gdal_i.lib 
IF  %GDAL%=="yes" SET USE_GDAL=/DUSE_GDAL

REM STEP f:  Set to "yes" to build only a gmt.dll version specially crafted for MATLAB
REM	     or pass ML as 1st argument to this script:
REM	     Adjust also MATLIB & MATINC variables to your own path
REM
SET forMATLAB="no"
IF  "%1%" == "ML" set forMATLAB="yes"
REM
IF  %forMATLAB%=="yes" (
SET MATLIB=/LIBPATH:C:\PROGRAMS\MATLAB\R2010B\extern\lib\win%BITS%\microsoft libmex.lib 
SET MATINC=-IC:\PROGRAMS\MATLAB\R2010B\extern\include
SET _MX_COMPAT=-DMX_COMPAT_32
SET TO_MATLAB=/DGMT_MATLAB
) ELSE (
SET MATLIB=
SET MATINC=
SET _MX_COMPAT=
SET TO_MATLAB=
)

REM STEP g: Specify if building in GMT4 compatibility mode
REM STEP    Set to "yes" to build in compatibility mode
SET COMPAT_MODE="yes"
REM
SET COMPAT=
IF  %COMPAT_MODE%=="yes" SET COMPAT=/DGMT_COMPAT

REM STEP h: Specify if building with PCRE support
REM 	    Set to "yes" to build with PCRE. The right path to PCRE_DIR must have been 
REM	    set already (top) or sent as input
SET PCRE__="yes"
SET PCRE_INC=
SET PCRE_LIB=
REM
SET PCRE=
IF  %PCRE__%=="yes" SET PCRE=/DHAVE_PCRE
IF  %PCRE__%=="yes" SET PCRE_INC="/I%PCRE_DIR%\include" 
IF  %PCRE__%=="yes" SET PCRE_LIB=%PCRE_DIR%\lib\pcre.lib 

REM STEP i: Optional use of Visual Leak Detector (VLD) by Dan Moulding, available at
REM	    http://vld.codeplex.com
REM	    Set to "yes" and adjust path to use VLD in debug configuration only:
SET VLD__="no"
SET VLD_DIR="C:\programs\Visual_Leak_Detector"
SET VLD_INC=
SET VLD_LIB=
SET VLD=
IF  %VLD__%=="yes" SET VLD=/DUSE_VLD
IF  %VLD__%=="yes" SET VLD_LIB=%VLD_DIR%\lib\Win%BITS%\vld.lib
IF  %VLD__%=="yes" SET VLD_INC="/I%VLD_DIR%\include"

REM STEP j: Set to "yes" to use Data-Alignment Memory Routines
REM	    See: http://msdn.microsoft.com/en-us/library/fs9stz4e(v=VS.71).aspx
SET MEM_ALIGNED=="no"
SET USE_MEM_ALIGNED=
IF  %MEM_ALIGNED%=="yes" SET USE_MEM_ALIGNED=/DUSE_MEM_ALIGNED

REM ----------------------------------------------------
REM STOP HERE - THE REST IS AUTOMATIC
REM ----------------------------------------------------

SET LDEBUG=
IF  %DEBUG%=="yes" SET LDEBUG=/debug
SET OPTIM=/O2 /DNDEBUG /MD /Ob2 /Zm1000
IF  %DEBUG%=="yes" SET OPTIM=/Z7 /DDEBUG

SET DLL_NETCDF=/DDLL_NETCDF
SET TR=/DTRIANGLE_D
SET NETCDF_INC=/I%NETCDF_DIR%\include

SET COMPFLAGS=/W3 /D_CRT_SECURE_NO_DEPRECATE /D_CRT_NONSTDC_NO_DEPRECATE /D_SCL_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /D_SECURE_SCL=0 /nologo
SET COPT=/I%cd% /DWIN32 %OPTIM% %TR% %DLL_NETCDF% /DDLL_PSL /DDLL_GMT %USE_GDAL% %GDAL_INC% %NETCDF_INC% %TO_MATLAB% %COMPFLAGS% %COMPAT% %PCRE% %PCRE_INC% %VLD_INC% %USE_MEM_ALIGNED% /DWINBITAGE=%BITS%

set LOPT=/nologo /dll /incremental:no %LDEBUG%

REM ----------------------------------------------------
ECHO STEP 1: Make PS library
REM ----------------------------------------------------

%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% pslib.c
link %LOPT% /out:psl.dll /implib:psl.lib pslib.obj
DEL pslib.obj

REM ----------------------------------------------------
ECHO STEP 2: Make GMT library
REM ----------------------------------------------------

%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmt_bcr.c gmt_cdf.c gmt_nc.c gmt_customio.c gmt_grdio.c gmt_notposix.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% %MATINC% gmt_init.c gmt_io.c gmt_map.c gmt_plot.c gmt_proj.c gmt_shore.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmt_fft.c gmt_stat.c gmt_calclock.c gmt_support.c gmt_vector.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmtapi_parse.c gmtapi_util.c gmt_regexp.c gmt_version.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% blockmean.c blockmedian.c blockmode.c colmath.c filter1d.c fitcircle.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmt2kml.c gmtconvert.c gmtdp.c gmtmath.c gmtselect.c gmtspatial.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmtstitch.c gmtvector.c gmtwhich.c grd2cpt.c grd2rgb.c grd2xyz.c grdblend.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% grdclip.c grdcut.c grdedit.c grdfft.c grdfilter.c grdgradient.c grdhisteq.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% grdinfo.c grdlandmask.c grdmask.c grdmath.c grdpaste.c grdproject.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% grdsample.c grdreformat.c grdtrack.c grdtrend.c grdvolume.c greenspline.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% kml2gmt.c makecpt.c mapproject.c minmax.c nearneighbor.c project.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% ps2raster.c sample1d.c spectrum1d.c splitxyz.c surface.c trend1d.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% trend2d.c triangulate.c xyz2grd.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% testio.c gmtaverage.c testapi.c
REM %CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% testgmt5.c testgmtio.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gmtdefaults.c gmtget.c gmtset.c grdcontour.c grdimage.c grdvector.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% grdview.c psbasemap.c psclip.c pscoast.c pscontour.c pshistogram.c psimage.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% psmask.c psrose.c psscale.c pstext.c pswiggle.c psxy.c psxyz.c pslegend.c

%CC% %COPT% /c /DNO_TIMER /DTRILIBRARY /DREDUCED /DCDT_ONLY triangle.c

REM ----------------------------------- SUPPLEMENTS ----------------------------------------------
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% dbase\grdraster.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gshhs\gshhs.c
REM %CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% gshhs\gshhs.c gshhs\gshhs_dp.c gshhs\gshhstograss.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% imgsrc\img2grd.c imgsrc\gmt_imgsubs.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% meca\nrutil.c meca\utilmeca.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% meca\psmeca.c meca\pspolar.c meca\psvelo.c meca\pscoupe.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% mgd77\mgd77.c mgd77\mgd77convert.c mgd77\mgd77info.c mgd77\mgd77list.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% mgd77\mgd77path.c mgd77\mgd77manage.c mgd77\mgd77magref.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% mgd77\mgd77sniffer.c mgd77\mgd77track.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% potential\redpol.c potential\xyzokb.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% segyprogs\segy_io.c segyprogs\pssegy.c segyprogs\pssegyz.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% segyprogs\segy2grd.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% sph\sph.c sph\sphtriangulate.c sph\sphdistance.c sph\sphinterpolate.c
REM %CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% spharmonics\grd2sph.c spharmonics\sph2grd.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% spotter\libspotter.c spotter\backtracker.c spotter\grdpmodeler.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% spotter\grdrotater.c spotter\grdspotter.c spotter\hotspotter.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% spotter\originator.c spotter\rotconverter.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% /I%cd%\mgd77 x2sys\x2sys.c x2sys\x2sys_init.c x2sys\x2sys_put.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% /I%cd%\mgd77 x2sys\x2sys_get.c x2sys\x2sys_binlist.c x2sys\x2sys_datalist.c
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% /I%cd%\mgd77 x2sys\x2sys_cross.c x2sys\x2sys_list.c x2sys\x2sys_report.c 
%CC% %COPT% /c /DDLL_EXPORT /DGMT_SHARE_PATH=%GMT_SHARE_PATH% /I%cd%\mgd77 x2sys\x2sys_solve.c x2sys\x2sys_merge.c
REM ----------------------------------------------------------------------------------------------

link %LOPT% /out:gmt.dll /implib:gmt.lib psl.lib %lib_netcdf% %GDAL_LIB% %MATLIB% %PCRE_LIB% %VLD_LIB% *.obj advapi32.lib

IF %forMATLAB%=="yes" DEL *.obj *.lib *.exp psl.dll
IF %forMATLAB%=="yes" GOTO fim

REM ----------------------------------------------------
ECHO STEP 3: Make GMT programs 
REM ----------------------------------------------------

set GMTLIB=gmt.lib %lib_netcdf% setargv.obj

%CC% %COPT% testio.c  %GMTLIB%
%CC% %COPT% gmtmercmap.c  %GMTLIB%
%CC% %COPT% psldemo.c %GMTLIB% psl.lib

%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_blockmean    /Feblockmean    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_blockmedian  /Feblockmedian  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_blockmode    /Feblockmode    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_colmath      /Fecolmath      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_filter1d     /Fefilter1d     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_fitcircle    /Fefitcircle    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmt2kml      /Fegmt2kml      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtconvert   /Fegmtconvert   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_gmtdefaults  /Fegmtdefaults  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtdp        /Fegmtdp        gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtget       /Fegmtget       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtmath      /Fegmtmath      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtselect    /Fegmtselect    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_gmtset       /Fegmtset       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtspatial   /Fegmtspatial   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtvector    /Fegmtvector    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gmtstitch    /Fegmtstitch    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdfilter    /Fegrdfilter    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grd2cpt      /Fegrd2cpt      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grd2rgb      /Fegrd2rgb      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grd2xyz      /Fegrd2xyz      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdblend     /Fegrdblend     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdclip      /Fegrdclip      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_grdcontour   /Fegrdcontour   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdcut       /Fegrdcut       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdedit      /Fegrdedit      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdfft       /Fegrdfft       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdgradient  /Fegrdgradient  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdhisteq    /Fegrdhisteq    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_grdimage     /Fegrdimage     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdinfo      /Fegrdinfo      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdlandmask  /Fegrdlandmask  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdmask      /Fegrdmask      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdmath      /Fegrdmath      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdpaste     /Fegrdpaste     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdproject   /Fegrdproject   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdreformat  /Fegrdreformat  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdsample    /Fegrdsample    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdtrend     /Fegrdtrend     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdtrack     /Fegrdtrack     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_grdvector    /Fegrdvector    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_grdview      /Fegrdview      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdvolume    /Fegrdvolume    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_greenspline  /Fegreenspline  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_kml2gmt      /Fekml2gmt      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_makecpt      /Femakecpt      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mapproject   /Femapproject   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_minmax       /Feminmax       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_nearneighbor /Fenearneighbor gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_project      /Feproject      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_ps2raster    /Feps2raster    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psbasemap    /Fepsbasemap    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psclip       /Fepsclip       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pscoast      /Fepscoast      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pscontour    /Fepscontour    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pshistogram  /Fepshistogram  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pslegend     /Fepslegend     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psimage      /Fepsimage      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psmask       /Fepsmask       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psrose       /Fepsrose       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psscale      /Fepsscale      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pstext       /Fepstext       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pswiggle     /Fepswiggle     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psxy         /Fepsxy         gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psxyz        /Fepsxyz        gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_sample1d     /Fesample1d     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_spectrum1d   /Fespectrum1d   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_splitxyz     /Fesplitxyz     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_surface      /Fesurface      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_testapi      /Fetestapi      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_trend1d      /Fetrend1d      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_trend2d      /Fetrend2d      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_triangulate  /Fetriangulate  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_xyz2grd      /Fexyz2grd      gmtprogram.c %GMTLIB%


REM ----------------------------------- SUPPLEMENTS -----------------------------------------------
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdraster    /Fegrdraster    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_gshhs        /Fegshhs        gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_img2grd      /Feimg2grd      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pscoupe      /Fepscoupe      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pspolar      /Fepspolar      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psmeca       /Fepsmeca       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_psvelo       /Fepsvelo       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77convert /Femgd77convert gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77info    /Femgd77info    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77list    /Femgd77list    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77manage  /Femgd77manage  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77path    /Femgd77path    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77sniffer /Femgd77sniffer gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_mgd77track   /Femgd77track   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_mgd77magref  /Femgd77magref  gmtprogram.c %GMTLIB%
%CC% %COPT% misc\dimfilter.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_redpol       /Feredpol       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_xyzokb       /Fexyzokb       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pssegy       /Fepssegy       gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMTPSL /DFUNC=GMT_pssegyz      /Fepssegyz      gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_segy2grd     /Fesegy2grd     gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_sphtriangulate /Fesphtriangulate gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_sphdistance    /Fesphdistance    gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_sphinterpolate /Fesphinterpolate gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_backtracker  /Febacktracker  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdpmodeler  /Fegrdpmodeler  gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdrotater   /Fegrdrotater   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_grdspotter   /Fegrdspotter   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_hotspotter   /Fehotspotter   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_originator   /Feoriginator   gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_rotconverter /Ferotconverter gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_binlist /Fex2sys_binlist  /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_cross   /Fex2sys_cross    /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_datalist /Fex2sys_datalist /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_get    /Fex2sys_get    /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_init   /Fex2sys_init   /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_put    /Fex2sys_put    /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_list   /Fex2sys_list   /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_report /Fex2sys_report /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_solve  /Fex2sys_solve  /I%cd%\mgd77 gmtprogram.c %GMTLIB%
%CC% %COPT% /DFUNC_MODE=GMTAPI_GMT    /DFUNC=GMT_x2sys_merge  /Fex2sys_merge gmtprogram.c %GMTLIB%
REM -----------------------------------------------------------------------------------------------

REM ----------------------------------------------------
ECHO STEP 4: Clean up and install executables and libraries
REM ----------------------------------------------------
:salto
DEL *.obj
MOVE *.exe %BINDIR%
MOVE *.lib %LIBDIR%
COPY *.h %INCDIR%
MOVE *.dll %BINDIR%
MOVE *.exp %LIBDIR%
IF %DEBUG%=="yes" MOVE *.pdb %BINDIR% 
DEL *.ilk *.idb
:fim