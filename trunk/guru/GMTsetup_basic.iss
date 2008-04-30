; Script generated by the Inno Setup Script Wizard.
; $Id: GMTsetup_basic.iss,v 1.6 2008-04-30 21:06:13 guru Exp $
; Creates Windows Setup file for the Basic GMT installation (no h/f coastlines, no PDF)

[Setup]
AppName=GMT
AppId=gmt4win
AppVerName=GMT 4.3.0
AppVersion=4.3.0
AppUpdatesURL=http://gmt.soest.hawaii.edu
DefaultDirName={sd}\programs\GMT
DefaultGroupName=GMT
AllowNoIcons=yes
UsePreviousAppDir=yes
LicenseFile=c:\GMT\guru\gpl.txt
InfoAfterFile=c:\GMT\guru\GMT_postinstall_message.txt
OutputBaseFilename=GMT4.3.0_basic_install
MinVersion=0,4.0.1381
OutputDir=c:\GMT\ftp

[Files]
Source: c:\GMT\COPYING; DestDir: {app}; Flags: ignoreversion
Source: c:\GMT\ChangeLog; DestDir: {app}; Flags: ignoreversion
Source: c:\GMT\bin\*.*; DestDir: {app}\bin; Flags: ignoreversion
Source: c:\NETCDF\bin\*.dll; DestDir: {app}\bin; Flags: ignoreversion
Source: c:\GMT\lib\*.*; DestDir: {app}\lib; Flags: ignoreversion
Source: c:\GMT\examples\*.*; DestDir: {app}\examples; Flags: ignoreversion
Source: c:\GMT\examples\ex01\*.*; DestDir: {app}\examples\ex01; Flags: ignoreversion
Source: c:\GMT\examples\ex02\*.*; DestDir: {app}\examples\ex02; Flags: ignoreversion
Source: c:\GMT\examples\ex03\*.*; DestDir: {app}\examples\ex03; Flags: ignoreversion
Source: c:\GMT\examples\ex04\*.*; DestDir: {app}\examples\ex04; Flags: ignoreversion
Source: c:\GMT\examples\ex05\*.*; DestDir: {app}\examples\ex05; Flags: ignoreversion
Source: c:\GMT\examples\ex06\*.*; DestDir: {app}\examples\ex06; Flags: ignoreversion
Source: c:\GMT\examples\ex07\*.*; DestDir: {app}\examples\ex07; Flags: ignoreversion
Source: c:\GMT\examples\ex08\*.*; DestDir: {app}\examples\ex08; Flags: ignoreversion
Source: c:\GMT\examples\ex09\*.*; DestDir: {app}\examples\ex09; Flags: ignoreversion
Source: c:\GMT\examples\ex10\*.*; DestDir: {app}\examples\ex10; Flags: ignoreversion
Source: c:\GMT\examples\ex11\*.*; DestDir: {app}\examples\ex11; Flags: ignoreversion
Source: c:\GMT\examples\ex12\*.*; DestDir: {app}\examples\ex12; Flags: ignoreversion
Source: c:\GMT\examples\ex13\*.*; DestDir: {app}\examples\ex13; Flags: ignoreversion
Source: c:\GMT\examples\ex14\*.*; DestDir: {app}\examples\ex14; Flags: ignoreversion
Source: c:\GMT\examples\ex15\*.*; DestDir: {app}\examples\ex15; Flags: ignoreversion
Source: c:\GMT\examples\ex16\*.*; DestDir: {app}\examples\ex16; Flags: ignoreversion
Source: c:\GMT\examples\ex17\*.*; DestDir: {app}\examples\ex17; Flags: ignoreversion
Source: c:\GMT\examples\ex18\*.*; DestDir: {app}\examples\ex18; Flags: ignoreversion
Source: c:\GMT\examples\ex19\*.*; DestDir: {app}\examples\ex19; Flags: ignoreversion
Source: c:\GMT\examples\ex20\*.*; DestDir: {app}\examples\ex20; Flags: ignoreversion
Source: c:\GMT\examples\ex21\*.*; DestDir: {app}\examples\ex21; Flags: ignoreversion
Source: c:\GMT\examples\ex22\*.*; DestDir: {app}\examples\ex22; Flags: ignoreversion
Source: c:\GMT\examples\ex23\*.*; DestDir: {app}\examples\ex23; Flags: ignoreversion
Source: c:\GMT\examples\ex24\*.*; DestDir: {app}\examples\ex24; Flags: ignoreversion
Source: c:\GMT\examples\ex25\*.*; DestDir: {app}\examples\ex25; Flags: ignoreversion
Source: c:\GMT\examples\ex26\*.*; DestDir: {app}\examples\ex26; Flags: ignoreversion
Source: c:\GMT\share\coast\*_c.cdf; DestDir: {app}\share\coast; Flags: ignoreversion
Source: c:\GMT\share\coast\*_l.cdf; DestDir: {app}\share\coast; Flags: ignoreversion
Source: c:\GMT\share\coast\*_i.cdf; DestDir: {app}\share\coast; Flags: ignoreversion
Source: c:\GMT\share\conf\*.*; DestDir: {app}\share\conf; Flags: ignoreversion
Source: c:\GMT\share\cpt\*.*; DestDir: {app}\share\cpt; Flags: ignoreversion
Source: c:\GMT\share\custom\*.*; DestDir: {app}\share\custom; Flags: ignoreversion
Source: c:\GMT\share\dbase\*.*; DestDir: {app}\share\dbase; Flags: ignoreversion
Source: c:\GMT\share\mgd77\*; DestDir: {app}\share\mgd77; Flags: ignoreversion
Source: c:\GMT\share\mgg\*.*; DestDir: {app}\share\mgg; Flags: ignoreversion
Source: c:\GMT\share\pattern\*.*; DestDir: {app}\share\pattern; Flags: ignoreversion
Source: c:\GMT\share\pslib\*.*; DestDir: {app}\share\pslib; Flags: ignoreversion
Source: c:\GMT\share\time\*.*; DestDir: {app}\share\time; Flags: ignoreversion
Source: c:\GMT\share\x2sys\*.*; DestDir: {app}\share\x2sys; Flags: ignoreversion
Source: c:\GMT\www\gmt\*.*; DestDir: {app}\www\gmt; Flags: ignoreversion
Source: c:\GMT\www\gmt\doc\html\*.*; DestDir: {app}\www\gmt\doc\html; Flags: ignoreversion
Source: c:\GMT\www\gmt\doc\html\GMT_Docs\*.*; DestDir: {app}\www\gmt\doc\html\GMT_Docs; Flags: ignoreversion
Source: c:\GMT\www\gmt\doc\html\GMT_Tutorial\*.*; DestDir: {app}\www\gmt\doc\html\tutorial; Flags: ignoreversion

[Messages]
WelcomeLabel2=This will install the basic components of [name/ver] on your computer, including HTML documentation and the crude, low, and intermediate GSHHS (version 1.10) coastline data.%n%nThe high and full resolution GSHHS coastlines may be installed separately and are not included in this installer.  Likewise, PDF documentation may also be installed separately.
LicenseAccepted=I have no choice but to &accept the agreement

[Registry]
Root: HKCU; Subkey: Environment; ValueType: string; ValueName: GMT_SHAREDIR; ValueData: {app}\share; Flags: createvalueifdoesntexist
Root: HKCU; Subkey: Environment; ValueType: string; ValueName: path; ValueData: {app}\bin; Flags: createvalueifdoesntexist uninsdeletekeyifempty
Root: HKCU; Subkey: Environment; ValueType: string; ValueName: path; ValueData: "{olddata};{app}\bin"; Flags: dontcreatekey uninsdeletekeyifempty
