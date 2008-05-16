ECHO OFF
REM	$Id: GMT_winbuild.bat,v 1.8 2008-05-16 04:10:14 guru Exp $
REM	Compiles GMT and builds installers under Windows.
REM	See separate GSHHS_winbuild.bat for GSHHS full+high installer
REM	Paul Wessel with help from Joaquim Luis
REM
REM	Assumptions:
REM	1. You have run make tar_all
REM	2. You have placed netcdf in C:\NETCDF
REM	3. INCLUDE, LIB, PATH have been set so that CL and
REM	   LIB will find the netcdf include and library
REM	4. HOME and GMTHOME has been set
REM	5. Inno Setup 5 has been installed and the path
REM	   to its command line tool is added to PATH
REM	6. 7zip has been installed and the path
REM	   to its command line tool is added to PATH

SET GVER=4.3.1

echo === 1. Get all GMT%GVER% bzipped tar balls and extract files...

cd C:\
copy Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp\GMT*.tar.bz2 C:\
copy Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp\GSHHS*coast.tar.bz2 C:\
7z x GMT*.tar.bz2
7z x GSHHS*coast.tar.bz2
7z x GMT*.tar
7z x GSHHS*coast.tar -oGMT%GVER% -aoa
del *.tar.bz2
del *.tar
rename GMT%GVER% GMT

echo === 2. Build the GMT executables, including supplements...

cd C:\GMT
mkdir bin
mkdir lib
cd src
call gmtinstall tri
call gmtsuppl

echo === 3. Run all the examples...

cd C:\GMT\examples
call do_examples
cd C:\GMT

echo === 4. Build the GMT Basic installer...

iscc /Q C:\GMT\guru\GMTsetup_basic.iss

echo === 5. Build the GMT PDF installer...

iscc /Q C:\GMT\guru\GMTsetup_pdf.iss

echo === 6. Place the GMT installers on macnut...

copy C:\GMT\ftp\*.exe Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp

echo === 7. DONE
ECHO ON
