#!/usr/bin/perl
#
#       $Id$
#
#	Parses the input provided by the install form
#	(Now in Bourne shell format)
#
#	Updated to work for multiple versions and passes browser
#	control to unique (temporary) parameter file on GMT server
#	You must start cron job on the server that will find and
#	delete all temporary files in .../gmt/gmttemp
#	that are older than 5 minutes.  The command for cron would be
#
#	entry for P.Wessel temp files (crontab on imina as root):
#	07 01 * * * find /export/imina2/apache222/htdocs/gmt/gmttemp -xdev -mtime +1 -type d -exec rm -r {} \; > /dev/null 2>&1
#
#	The temporary files are placed in <PID>/GMTparam.txt

$webmaster = "gmt\@soest\.hawaii\.edu";

# Create unique filename for temp file:

$PID		= $$;
$OUT          = "/export/imina2/httpd/htdocs/gmt/gmttemp/" . $PID . "/GMTparam.txt";
$FOUT          = $PID . "/GMTparam.txt";
$PDIR           = "/export/imina2/httpd/htdocs/gmt/gmttemp/" . $PID;

mkdir $PDIR;

&parse_form_data (*gmt_form);

# Assign internal variables for each form item:

$form_version	= $gmt_form{'form_version'};
$unit		= $gmt_form{'radio_unit'};
$flock		= $gmt_form{'radio_flock'};
$cdf		= $gmt_form{'radio_netcdf'};
$ftpmode	= $gmt_form{'radio_ftpmode'};
$cdf_path	= $gmt_form{'netcdf_dir'};
$gdal		= $gmt_form{'radio_gdal'};
$gdal_path	= $gmt_form{'gdal_dir'};
$site		= $gmt_form{'radio_site'};
$get_src	= $gmt_form{'checkbox_src'};
$get_share	= $gmt_form{'checkbox_share'};
$get_coast	= $gmt_form{'checkbox_coast'};
$get_doc	= $gmt_form{'checkbox_doc'};
$get_suppl	= $gmt_form{'checkbox_suppl'};
$get_high	= $gmt_form{'checkbox_high'};
$get_full	= $gmt_form{'checkbox_full'};
$use_triangle	= $gmt_form{'radio_triangle'};
$libtype	= $gmt_form{'radio_link'};
$cc		= $gmt_form{'cc'};
$custom_cc	= $gmt_form{'custom_cc'};
$gmt_mp		= $gmt_form{'checkbox_mp'};
$gmt_64		= $gmt_form{'checkbox_64'};
$gmt_univ	= $gmt_form{'checkbox_univ'};
$make		= $gmt_form{'make'};
$custom_make	= $gmt_form{'custom_make'};
$gmt_prefix	= $gmt_form{'gmt_prefix'};
$gmt_bin	= $gmt_form{'gmt_bin'};
$gmt_lib	= $gmt_form{'gmt_lib'};
$gmt_include	= $gmt_form{'gmt_include'};
$gmt_share	= $gmt_form{'gmt_share'};
$gmt_man	= $gmt_form{'gmt_man'};
$gmt_doc	= $gmt_form{'gmt_doc'};
$gmt_sharedir	= $gmt_form{'gmt_sharedir'};
$gmt_coast	= $gmt_form{'radio_coast'};
$gmt_cli_dir	= $gmt_form{'gmt_cli_dir'};
$gmt_h_dir	= $gmt_form{'gmt_h_dir'};
$gmt_f_dir	= $gmt_form{'gmt_f_dir'};
$get_dbase	= $gmt_form{'checkbox_dbase'};
$get_gshhs	= $gmt_form{'checkbox_gshhs'};
$get_imgsrc	= $gmt_form{'checkbox_imgsrc'};
$get_meca	= $gmt_form{'checkbox_meca'};
$get_mgd77	= $gmt_form{'checkbox_mgd77'};
$get_mex	= $gmt_form{'checkbox_mex'};
$mex_type	= $gmt_form{'radio_mex'};
$get_misc	= $gmt_form{'checkbox_misc'};
$get_potential	= $gmt_form{'checkbox_potential'};
$get_segyprogs	= $gmt_form{'checkbox_segyprogs'};
$get_sph	= $gmt_form{'checkbox_sph'};
$get_spotter	= $gmt_form{'checkbox_spotter'};
$get_x2sys	= $gmt_form{'checkbox_x2sys'};
$get_xgrid	= $gmt_form{'checkbox_xgrid'};
$matlab_dir	= $gmt_form{'matlab_dir'};
$mex_mdir	= $gmt_form{'mex_mdir'};
$mex_xdir	= $gmt_form{'mex_xdir'};
$delete		= $gmt_form{'checkbox_delete'};
$run		= $gmt_form{'checkbox_run'};

$now		= `date`;
chop($now);


# Open temp file

open (FILE, ">" . $OUT) || die "Sorry, cound not create tmp file\n";
print FILE <<EOF;
# This file contains parameters needed by the install script
# install_gmt for GMT Version 5.0.0b.  Give this file
# as the argument to the install_gmt script and the whole
# installation process can be placed in the background.
# Default answers will be selected where none is given.
# You can edit the values, but do not remove definitions!
#
# Assembled by gmt_install_form.html, $form_version
# Processed by install_gmt_form.pl $Revision$, on
#
#	$now
#
# Do NOT add any spaces around the = signs.  The
# file MUST conform to Bourne shell syntax
#---------------------------------------------
#	SYSTEM UTILITIES
#---------------------------------------------
EOF

@k = split (/\s+/, $make);
if ($k[0] eq "1.") {
	print FILE "GMT_make=make\n";
}
else {
	print FILE "GMT_make=", $custom_make, "\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	NETCDF SECTION\n";
print FILE "#---------------------------------------------\n";
if ($cdf eq "get") {
	print FILE "netcdf_ftp=y\n";
	print FILE "netcdf_install=y\n";
}
elsif ($cdf eq "install") {
	print FILE "netcdf_ftp=n\n";
	print FILE "netcdf_install=y\n";
}
else {
	print FILE "netcdf_ftp=n\n";
	print FILE "netcdf_install=n\n";
}
print FILE "netcdf_path=", $cdf_path, "\n";
print FILE "passive_ftp=";
if ($ftpmode eq "passive") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "#---------------------------------------------\n";
print FILE "#	GDAL SECTION\n";
print FILE "#---------------------------------------------\n";
if ($gdal eq "yes") {
	print FILE "use_gdal=y\n";
	print FILE "gdal_path=", $gdal_path, "\n";
}
else {
	print FILE "use_gdal=n\n";
	print FILE "gdal_path=\n";	
}
print FILE "#---------------------------------------------\n";
print FILE "#	GMT FTP SECTION\n";
print FILE "#---------------------------------------------\n";
if ($site eq "99") {
	print FILE "GMT_ftp=n\n";
}
else {
	print FILE "GMT_ftp=y\n";
}
print FILE "GMT_ftpsite=", $site, "\n";
print FILE "GMT_get_src=";
if ($get_src eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_share=";
if ($get_share eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_coast=";
if ($get_coast eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_high=";
if ($get_high eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_full=";
if ($get_full eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_suppl=";
if ($get_suppl eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_get_doc=";
if ($get_doc eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	GMT SUPPLEMENTS SELECT SECTION\n";
print FILE "#---------------------------------------------\n";

print FILE "GMT_suppl_dbase=";
if ($get_dbase eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_gshhs=";
if ($get_gshhs eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_imgsrc=";
if ($get_imgsrc eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_meca=";
if ($get_meca eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_mex=";
if ($get_mex eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_mex_type=";
if ($mex_type eq "matlab") {
	print FILE "matlab\n";
}
elsif ($mex_type eq "octave") {
	print FILE "octave\n";
}
else {
	print FILE "NONE\n";
}
print FILE "GMT_suppl_mgd77=";
if ($get_mgd77 eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_misc=";
if ($get_misc eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_potential=";
if ($get_potential eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_segyprogs=";
if ($get_segyprogs eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_sph=";
if ($get_sph eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_spotter=";
if ($get_spotter eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_x2sys=";
if ($get_x2sys eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_suppl_xgrid=";
if ($get_xgrid eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	GMT ENVIRONMENT SECTION\n";
print FILE "#---------------------------------------------\n";

if ($unit eq "SI") {
	print FILE "GMT_si=y\n";
}
else {
	print FILE "GMT_si=n\n";
}

if ($gmt_sharedir eq "" && $gmt_share ne "") {
	$gmt_sharedir =$gmt_share;
}
if ($gmt_prefix eq "" && $gmt_bin ne "") {
	$gmt_prefix =~ s#/bin$##;	# Remove trailing /bin
}

print FILE "GMT_prefix=", $gmt_prefix, "\n";
print FILE "GMT_bin=", $gmt_bin, "\n";
print FILE "GMT_lib=", $gmt_lib, "\n";
print FILE "GMT_share=", $gmt_share, "\n";
print FILE "GMT_include=", $gmt_include, "\n";
print FILE "GMT_man=", $gmt_man, "\n";
print FILE "GMT_doc=", $gmt_doc, "\n";
print FILE "GMT_sharedir=", $gmt_sharedir, "\n";

if ($gmt_coast eq "all") {
	print FILE "GMT_dir_full=\nGMT_dir_high=\nGMT_dir_cli=\n";
}
else {
	print FILE "GMT_dir_full=", $gmt_full_dir, "\n";
	print FILE "GMT_dir_high=", $gmt_high_dir, "\n";
	print FILE "GMT_dir_cli=", $gmt_cli_dir, "\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	COMPILING & LINKING SECTION\n";
print FILE "#---------------------------------------------\n";

if ($libtype eq "Static") {
	print FILE "GMT_sharedlib=n\n";
}
else {
	print FILE "GMT_sharedlib=y\n";
}

@k = split (/\s+/, $cc);
if ($k[0] eq "1.") {
	print FILE "GMT_cc=cc\n";
}
elsif ($k[0] eq "2.") {
	print FILE "GMT_cc=gcc\n";
}
else {
	print FILE "GMT_cc=", $custom_cc, "\n";
}
print FILE "GMT_MP=";
if ($gmt_mp eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_64=";
if ($gmt_64 eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
print FILE "GMT_UNIV=";
if ($gmt_univ eq "on") {
	print FILE "y\n";
}
else {
	print FILE "n\n";
}
if ($flock eq "Lock") {
	print FILE "GMT_flock=y\n";
}
else {
	print FILE "GMT_flock=n\n";
}

if ($use_triangle eq "Shewchuk") {
	print FILE "GMT_triangle=y\n";
}
else {
	print FILE "GMT_triangle=n\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	TEST & print FILE SECTION\n";
print FILE "#---------------------------------------------\n";

if ($run eq "on") {
	print FILE "GMT_run_examples=y\n";
}
else {
	print FILE "GMT_run_examples=n\n";
}
if ($delete eq "on") {
	print FILE "GMT_delete=y\n";
}
else {
	print FILE "GMT_delete=n\n";
}

print FILE "#---------------------------------------------\n";
print FILE "#	MEX SECTION\n";
print FILE "#---------------------------------------------\n";

if ($matlab_dir ne "" && $mex_type eq "matlab") {
	print FILE "MATDIR=", $matlab_dir, "\n";
}
if ($mex_mdir ne "") {
	print FILE "MEX_MDIR=", $mex_mdir, "\n";
}
if ($mex_xdir ne "") {
	print FILE "MEX_XDIR=", $mex_xdir, "\n";
}

close (FILE);

# Create return plain text file for browser with SSI to get the temp file

print "Content-type: text/html", "\n";
print "Status: 200 OK", "\n\n";
print "<META HTTP-EQUIV=\"refresh\" CONTENT=\"0; url=http://gmt.soest.hawaii.edu/gmttemp/$FOUT\">";

exit(0);

sub parse_form_data
{
	local (*FORM_DATA) = @_;

	local ( $request_method, $query_string, @key_value_pairs, $key_value, $key, $value);

	$request_method = $ENV{'REQUEST_METHOD'};

	if ($request_method eq "GET") {
		$query_string = $ENV{'QUERY_STRING'};
	} elsif ($request_method eq "POST") {
		read (STDIN, $query_string, $ENV{'CONTENT_LENGTH'});
	} else {
		&return_error (500, "Server Error", "Server uses unsupported method");
	}

	@key_value_pairs = split (/&/, $query_string);

	foreach $key_value (@key_value_pairs) {

		($key, $value) = split (/=/, $key_value);
		$value =~ tr/+/ /;
		$value =~ s/%([\dA-Fa-f][\dA-Fa-f])/pack ("C", hex ($1))/eg;

		if (defined ($FORM_DATA{$key})) {
			$FORM_DATA{$key} = join ("+", $FORM_DATA{$key}, $value);
		} else {
			$FORM_DATA{$key} = $value;
		}
	}
}

sub return_error
{
	local ($status, $keyword, $message) = @_;

	print "Content-type: text/html", "\n";
	print "Status: ", $status, " ", $keyword, "\n\n";
	
	print <<End_of_Error;

<title>CGI Program - Unexpected Error</title>
<h1>$keyword</h1>
<hr>$message<hr>
Please contact $webmaster for more information.

End_of_Error

	exit (1);
}
