; Script generated by the Inno Setup Script Wizard.
; $Id: GMTsetup_hfcoast.iss,v 1.2 2008-04-26 04:34:47 guru Exp $
; Creates Setup file for the optional h/f coastlines
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=GSHHS
AppId=gmt4win
AppVersion=1.10
AppVerName=GSHHS 1.10
AppUpdatesURL=http://gmt.soest.hawaii.edu
DefaultDirName={sd}\programs\GMT
DefaultGroupName=GMT
AllowNoIcons=true
Compression=bzip
LicenseFile=c:\GMT\guru\gpl.txt
OutputBaseFilename=GSHHS1.10_highfull
MinVersion=0,4.0.1381
OutputDir=c:\GMT\ftp

[Files]
Source: c:\GMT\COPYING; DestDir: {app}; Flags: ignoreversion
Source: c:\GMT\share\coast\*_h.cdf; DestDir: {app}\share\coast; Flags: ignoreversion
Source: c:\GMT\share\coast\*_f.cdf; DestDir: {app}\share\coast; Flags: ignoreversion

[Messages]
WelcomeLabel2=This will install [name/ver] high and full resolution GSHHS coastlines (version 1.10) on your computer.%n%nYou should already have completed the basic GMT installation.
LicenseAccepted=I have no choice but to &accept the agreement

[Registry]

