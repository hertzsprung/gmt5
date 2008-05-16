ECHO OFF
REM	$Id: GSHHS_winbuild.bat,v 1.2 2008-05-16 04:10:14 guru Exp $
REM	Builds installer for GSHHS under Windows
REM	Paul Wessel with help from Joaquim Luis
REM
REM	Assumptions:
REM	1. You have run make tar_high tar_full
REM	2. Inno Setup 5 has been installed and the path
REM	   to its command line tool is added to PATH
REM	3. 7zip has been installed and the path
REM	   to its command line tool is added to PATH

SET GVER=4.3.1
SET GSHHS=1.10

echo === 1. Get all GSHHS %GSHHS% bzipped tar balls and extract files...

cd C:\
copy Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp\GSHHS*high.tar.bz2 C:\
copy Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp\GSHHS*full.tar.bz2 C:\
7z x GSHHS*high.tar.bz2
7z x GSHHS*full.tar.bz2
7z x GSHHS*high.tar -oGMT%GVER% -aoa
7z x GSHHS*full.tar -oGMT%GVER% -aoa
del GSHHS*.tar.bz2
del GSHHS*.tar

echo === 2. Build the GSHHS full/high installer...

iscc /Q C:\GMT\guru\GMTsetup_hfcoast.iss

echo === 3. Place the GMT installers on macnut...

copy C:\GMT\ftp\GSHHS*.exe Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp

echo === 4. DONE
ECHO ON
