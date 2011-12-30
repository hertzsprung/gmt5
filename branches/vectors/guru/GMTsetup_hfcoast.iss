; Script generated by the Inno Setup Script Wizard.
; $Id$
; Creates Setup file for the optional h/f coastlines
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=GSHHS
AppId=gmt4win
AppVersion=2.2.0
AppVerName=GSHHS 2.2.0
AppUpdatesURL=http://gmt.soest.hawaii.edu
DefaultDirName={sd}\programs\GMT
DefaultGroupName=GMT
AllowNoIcons=true
Compression=bzip
LicenseFile=Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\guru\gpl.txt
OutputBaseFilename=GSHHS2.2.0_highfull_install
MinVersion=0,4.0.1381
OutputDir=Y:\UH\RESEARCH\PROJECTS\GMTdev\GMT\ftp

[Files]
Source: c:\GMT\LICENSE.TXT; DestDir: {app}; Flags: ignoreversion
Source: c:\GMT\share\coast\*_h.cdf; DestDir: {app}\share\coast; Flags: ignoreversion
Source: c:\GMT\share\coast\*_f.cdf; DestDir: {app}\share\coast; Flags: ignoreversion

[Messages]
WelcomeLabel2=This will install [name/ver] high and full resolution GSHHS coastlines (version 2.2.0) on your computer.%n%nYou should already have completed the basic GMT installation.
LicenseAccepted=I have no choice but to &accept the agreement

[Registry]

