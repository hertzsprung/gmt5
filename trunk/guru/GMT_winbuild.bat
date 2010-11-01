ECHO OFF
REM	$Id: GMT_winbuild.bat,v 1.43 2010-11-01 23:12:31 guru Exp $
REM	Compiles GMT and builds installers under Windows.
REM	See separate GSHHS_winbuild.bat for GSHHS full+high installer
REM	Paul Wessel with help from Joaquim Luis
REM
REM	Assumptions:
REM	1. You have run make tar_all tar_coast
REM	2. You have placed netcdf in C:\NETCDF
REM	3. You have C:\GMTdev with dirs INFO and INSTALLERS
REM	4. Inno Setup 5 has been installed and the path
REM	   to its command line tool is added to PATH
REM	5. 7zip has been installed and the path
REM	   to its command line tool is added to PATH
REM	6. FWTools2.4.7 has been installed under C:\programs
REM	   and the path to its DLL is added to PATH

SET GVER=4.5.5
SET GSHHS=2.1.0

IF "%1%" == "home" (
	SET GMTDIR=W:\RESEARCH\PROJECTS\GMTdev\GMT4
) ELSE (
	SET GMTDIR=%1%:\UH\RESEARCH\PROJECTS\GMTdev\GMT4
)

echo === 1. Get all GMT%GVER% bzipped tar balls and extract files...

C:
cd C:\GMTdev
copy %GMTDIR%\ftp\GMT%GVER%*.tar.bz2 C:\GMTdev\
copy %GMTDIR%\ftp\GSHHS%GSHHS%_*.tar.bz2 C:\GMTdev\
7z x GMT*.tar.bz2
7z x GSHHS*.tar.bz2
7z x GMT*.tar -aoa
7z x GSHHS*.tar -oGMT%GVER% -aoa
del *.tar.bz2
del *.tar
rename GMT%GVER% GMT
copy %GMTDIR%\src\gmt_version.h C:\GMTdev\GMT\src
copy %GMTDIR%\src\gmt_notposix.h C:\GMTdev\GMT\src
copy %GMTDIR%\share\conf\gmt.conf.win C:\GMTdev\GMT\share\conf\gmt.conf
copy %GMTDIR%\share\conf\gmtdefaults_SI C:\GMTdev\GMT\share\conf
copy %GMTDIR%\share\conf\gmtdefaults_US C:\GMTdev\GMT\share\conf

mkdir C:\GMTdev\INFO
mkdir C:\GMTdev\INSTALLERS

copy %GMTDIR%\guru\GMT_postinstall_message.txt C:\GMTdev\INFO

echo === 2. Build the GMT executables, including supplements...

set INCLUDE=%INCLUDE%;C:\NETCDF\INCLUDE
set LIB=%LIB%;C:\NETCDF\LIB

cd C:\GMTdev\GMT
mkdir bin
mkdir lib
mkdir include
cd src
call gmtinstall tri no
call gmtsuppl

echo === 3. Run all the examples...

set GMT_SHAREDIR=C:\GMTdev\GMT\share
set OLDPATH=%PATH%
set PATH=C:\GMTdev\GMT\bin;C:\NETCDF\bin;%OLDPATH%

cd C:\GMTdev\GMT\share\doc\gmt\examples
call do_examples
cd C:\GMTdev\GMT

echo === 4. Remove all the examples PS files...

cd C:\GMTdev\GMT\share\doc\gmt\examples
for %%d in (01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30) do del ex%%d\*.ps
cd C:\GMTdev\GMT

echo === 5. Build the GMT Basic installer...

iscc /Q %GMTDIR%\guru\GMTsetup_basic.iss

echo === 6. Rebuild the GMT executables enabling GDAL support...

cd C:\GMTdev\GMT\src
call gmtinstall tri gdal
call gmtsuppl

echo === 7. Build the GMT+GDAL installer...

iscc /Q %GMTDIR%\guru\GMTsetup_gdal.iss

echo === 8. Build the GMT PDF installer...

iscc /Q %GMTDIR%\guru\GMTsetup_pdf.iss

echo === 10. PLACE INSTALLERS in ftp dir

cd C:\GMTdev\
copy INSTALLERS\*.exe %GMTDIR%\ftp

set PATH=%OLDPATH%
 
echo === 9. DONE

ECHO ON
