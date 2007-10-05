#!/bin/sh
#
#	$Id: install_gmt.sh,v 1.113 2007-10-05 00:57:05 guru Exp $
#
#	Automatic installation of GMT
#	Suitable for the Bourne shell (or compatible)
#
#	Paul Wessel
#	02-OCT-2007
#--------------------------------------------------------------------------------
# GLOBAL VARIABLES
NETCDF_VERSION=3.6.2
VERSION=4.2.1
GSHHS=1.9
GMT_FTP_TEST=0
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#	FUNCTIONS
#--------------------------------------------------------------------------------
# Get the functionality of echo -n
#--------------------------------------------------------------------------------
if [ x`echo -n` = x ]; then	# echo -n works
	echon()
	{
		echo -n "$*" 
	}
elif [ x`echo -e` = x ]; then	# echo -e works
	echon()
	{
		echo -e "$*"'\c'
	}
else				# echo with escapes better work
	echon()
	{
		echo "$*"'\c'
	}
fi
# Question poser, return answer
#--------------------------------------------------------------------------------
get_answer()
{
# Arg1 is question
echon "==> $1: " >&2
read answer
echo $answer
}
#--------------------------------------------------------------------------------
# Question poser with default answer, return answer
#--------------------------------------------------------------------------------
get_def_answer()
{
# Arg1 is question, Argv2 is default answer
echon "==> $1 [$2]: " >&2
read answer
echo $answer $2 | awk '{print $1}'
}
which_zip()
{
if [ `bzip2 -h 2>&1 | wc -l` -lt 10 ]; then	# Dont have bzip2
	if [ `gzip -h 2>&1 | wc -l` -lt 10 ]; then	# Dont have gzip
		echo "ERROR: Neither gzip or bzip2 installed - exits" >&2
		exit
	else
		use=gzip
	fi
else				# Try gzip
	use=bzip2
fi
echo $use
}
#--------------------------------------------------------------------------------
# INTERACTIVE GMT PARAMETER REQUEST SESSION
#--------------------------------------------------------------------------------
prep_gmt()
{
#--------------------------------------------------------------------------------
cat << EOF > gmt_install.ftp_site
1. SOEST, U of Hawaii [GMT Home], Honolulu, Hawaii, USA
2. NOAA, Lab for Satellite Altimetry, Silver Spring, Maryland, USA
3. IRIS, Incorporated Research Institutions for Seismology, Seattle, Washington, USA
4. IAG-USP, Dept of Geophysics, U. of Sao Paulo, BRAZIL
5. Dept of Geosciences, U of Oslo, NORWAY
6. Goodie Domain Service, Vienna U of Techology, AUSTRIA
7. Tokai U, Shimizu, JAPAN
8. Charles Sturt U, Albury, AUSTRALIA
EOF
# Order (1-10) is 1:src, 2:share, 3:coast, 4:high, 5:full, 6:suppl, 7:scripts
#		  8:pdf, 9:web, 10:tut
cat << EOF > gmt_install.ftp_bzsizes
1.0
0.05
4.1
8.6
28.0
0.8
2.8
9.7
2.8
0.9
EOF
cat << EOF > gmt_install.ftp_gzsizes
1.3
0.05
4.4
10.7
46.1
0.8
3.4
9.7
3.0
1.2
EOF
cat << EOF >&2
====>>>> Interactive installation of GMT <<<<====
		  
We first need a questions and answer session to
determine how and where GMT is to be installed.
Then, when all parameters have been assembled,
we will run the installation (unless you chose
-n when starting this script).

This script will install GMT version $VERSION.

EOF

topdir=`pwd`
os=`uname -s`
#--------------------------------------------------------------------------------
# See if user has defined NETCDFHOME, if so use it as default path
#--------------------------------------------------------------------------------

netcdf_path=${NETCDFHOME:-}

#--------------------------------------------------------------------------------
#	DETERMINE WHICH OF GZIP AND BZIP2 FILES TO USE
#--------------------------------------------------------------------------------

GMT_expand=`which_zip`
if [ "$GMT_expand" = "bzip2" ]; then	# Use bzip2
	suffix="bz2"
	expand="bzip2 -dc"
	sizes=gmt_install.ftp_bzsizes
	echo "+++ Will expand *.bz2 files made with bzip2 +++" >&2
elif [ "$GMT_expand" = "gzip" ]; then
	suffix="gz"
	expand="gzip -dc"
	sizes=gmt_install.ftp_gzsizes
cat << EOF >&2
+++ Will expand *.gz files make with gzip +++
    (Consider installing bzip2 (sourceware.cygnus.com/bzip2/index.html)
     since bzip2 files are considerably smaller\!)
EOF
fi

#--------------------------------------------------------------------------------
#	MAKE UTILITY
#--------------------------------------------------------------------------------

GMT_make=`get_def_answer "Enter make utility to use" "make"`

#--------------------------------------------------------------------------------
#	FTP MODE
#--------------------------------------------------------------------------------
passive_ftp=n
if [ $do_ftp_qa -eq 1 ]; then
	cat << EOF >&2

If you are behind a firewall you are unlikely to have permission to initiate a
normal ftp session (which involves the server connecting back to the client).
If so, you may want to select passive ftp mode.

EOF
	passive_ftp=`get_def_answer "Do you want passive ftp transmission (y/n)" "n"`
fi

#--------------------------------------------------------------------------------
#	NETCDF SETUP
#--------------------------------------------------------------------------------

answer=`get_def_answer "Have you installed netcdf (version 3.6 or later)? (y/n)" "y"`
if [ "$answer" = "n" ]; then	# Must install netcdf one way or the other
	netcdf_path=""
	netcdf_ftp=n
	if [ $do_ftp_qa -eq 1 ]; then
		answer=`get_def_answer "Do you want me to ftp it for you? (y/n)" "y"`
		if [ "$answer" = "n" ]; then
			answer=`get_def_answer "Do you have netcdf.tar.{Z,bz2,gz} in $topdir? (y/n)" "y"`
			if [ "$answer" = "n" ]; then
				echo "Please ftp or install netcdf and then rerun install_gmt" >&2
				exit
			fi
		else
			netcdf_ftp=y
		fi
	fi
	netcdf_install=y
	if [ "$netcdf_ftp" = "n" ]; then	# Check that the files are actually there
		ok=0
		if [ -f netcdf.tar.Z ]; then
			ok=1
		elif [ -f netcdf.tar.bz2 ] && [ "$GMT_expand" = "bzip2" ]; then
			ok=1
		elif [ -f netcdf.tar.gz ] && [ "$GMT_expand" = "gzip" ]; then
			ok=1
		fi
		if [ $ok -eq 0 ]; then
			echo "netcdf.tar.{Z,bz2,gz} not in $topdir" >&2
			echo "Please ftp netcdf or have me do it" >&2
			exit
		fi
	fi
else
	def=${NETCDFHOME:-/usr/local/netcdf-$NETCDF_VERSION}
	netcdf_path=`get_def_answer "Enter directory with netcdf lib and include" "$def"`
	netcdf_ftp=n
	netcdf_install=n
fi
	
#--------------------------------------------------------------------------------
#	GMT FTP SECTION
#--------------------------------------------------------------------------------

GMT_get_src=d
GMT_get_share=d
GMT_get_coast=d
GMT_get_scripts=d
GMT_get_suppl=d
GMT_get_pdf=d
GMT_get_man=d
GMT_get_web=d
GMT_get_tut=d
GMT_get_high=d
GMT_get_full=d
GMT_triangle=n
GMT_ftpsite=1
GMT_ftp=n
if [ $do_ftp_qa -eq 1 ]; then
	GMT_ftp=`get_def_answer "Get any of the GMT version $VERSION archives via ftp? (y/n)" "y"`
fi

if [ "$GMT_ftp" = "y" ]; then
	cat << EOF >&2

We offer $N_FTP_SITES different ftp sites.  Choose the one nearest
you in order to minimize net traffic and transmission times.
The sites are:

EOF
	cat gmt_install.ftp_site >&2
	echo " " >&2
	GMT_ftpsite=`get_def_answer "Enter your choice" "1"`
	if [ $GMT_ftpsite -eq 0 ]; then
		echo " [Special GMT guru testing site selected]" >&2
		GMT_ftpsite=1
		GMT_FTP_TEST=1
	elif [ $GMT_ftpsite -lt 0 ] || [ $GMT_ftpsite -gt $N_FTP_SITES ]; then
		GMT_ftpsite=1
		echo " Error in assigning site, using default site." >&2
	else
		echo " You selected site number $GMT_ftpsite:" >&2
		sed -n ${GMT_ftpsite}p gmt_install.ftp_site >&2
	fi
	echo " " >&2
	ftp_ip=`sed -n ${GMT_ftpsite}p gmt_install.ftp_ip`
	is_dns=`sed -n ${GMT_ftpsite}p gmt_install.ftp_dns`
	if [ $is_dns -eq 1 ]; then
		cat << EOF >&2
		
This anonymous ftp server $ftp_ip only accepts
connections from computers on the Internet that are registered
in the Domain Name System (DNS).  If you encounter a problem
connecting because your computer is not registered, please
either use a different computer that is registered or see your
computer systems administrator (or your site DNS coordinator)
to register your computer.

EOF
	fi

	echo " The first three archives are required for a minimal GMT install" >&2
	echo " " >&2

	size=`sed -n 1p $sizes`
	GMT_get_src=`get_def_answer "Want the program source archive [$size Mb] (y/n)?" "y"`
	size=`sed -n 2p $sizes`
	GMT_get_share=`get_def_answer "Want the shared run-time files (cpt, patterns) [$size Mb] (y/n)?" "y"`
	size=`sed -n 3p $sizes`
	GMT_get_coast=`get_def_answer "Want the basic support data (coastlines) [$size Mb] (y/n)?" "y"`

	echo " " >&2
	echo " The next six archives are optional but recommended for a typical GMT install" >&2
	echo " " >&2

	size=`sed -n 6p $sizes`
	GMT_get_suppl=`get_def_answer "Want optional GMT supplemental programs [$size Mb] (y/n)?" "y"`
	size=`sed -n 7p $sizes`
	GMT_get_scripts=`get_def_answer "Want optional GMT example scripts and data [$size Mb] (y/n)?" "y"`
	size=`sed -n 8p $sizes`
	GMT_get_pdf=`get_def_answer "Want optional GMT Documentation (PDF version) [$size Mb] (y/n)?" "y"`
	size=`sed -n 9p $sizes`
	GMT_get_web=`get_def_answer "Want optional GMT Web Documentation (HTML of all Docs) [$size Mb] (y/n)?" "y"`
	size=`sed -n 10p $sizes`
	GMT_get_tut=`get_def_answer "Want optional GMT tutorial data sets [$size Mb] (y/n)?" "y"`

	echo " " >&2
	echo " The next two archives are larger and contain more accurate coastline data:" >&2
	echo " " >&2

	size=`sed -n 4p $sizes`
	GMT_get_high=`get_def_answer "Want optional high resolution coastline data [$size Mb] (y/n)?" "y"`
	size=`sed -n 5p $sizes`
	GMT_get_full=`get_def_answer "Want optional full resolution coastline data [$size Mb] (y/n)?" "y"`

			     	
	echo " " >&2
	echo "GMT can use two different algorithms for Delauney triangulation." >&2
	echo " " >&2
	echo "   Shewchuk [1996]: Modern and very fast, copyrighted." >&2
	echo "   Watson [1982]  : Older and slower, public domain." >&2
	echo " " >&2
	echo "Because of the copyright, GMT uses Watson's routine by default." >&2
	echo " " >&2
	GMT_triangle=`get_def_answer "Use optional Shewchuk's triangulation routine (y/n)?" "n"`
else
	echo " " >&2
	echo "Since ftp mode is not selected, the install procedure will" >&2
	echo "assume the compressed archives are in the current directory." >&2
	echo "GMT can use two different algorithms for Delauney triangulation." >&2
	echo " " >&2
	echo "   Shewchuk [1996]: Modern and very fast, copyrighted." >&2
	echo "   Watson [1982]  : Older and slower, public domain." >&2
	echo " " >&2
	echo "Because of the copyright, GMT uses Watson's routine by default." >&2
	echo "However, most will want to use the optional Shewchuk routine." >&2
	GMT_triangle=`get_def_answer "Use optional Shewchuk's triangulation routine (y/n)?" "y"`
fi
	
cat << EOF >&2

The installation will install all GMT components in several subdirectories
under one root directory. On most Unix systems this root directory will be
something like /usr/local or /sw, under which the installation will add
bin, lib, share, etc.  Below you are asked to select to location of each
of the subdirectories.

EOF
GMT_bin=`get_def_answer "Directory for GMT executables?" "$topdir/GMT${VERSION}/bin"`
GMT_prefix=`echo $GMT_bin | sed 's!/bin!!'`
GMT_lib=`get_def_answer "Directory for GMT linkable libraries?" "$GMT_prefix/lib"`
GMT_include=`get_def_answer "Directory for GMT include files?" "$GMT_prefix/include"`
GMT_share=`get_def_answer "Directory for GMT data resources?" "$GMT_prefix/share"`

cat << EOF >&2

Unix man pages are usually stored in /usr/man/manX, where X is
the relevant man section.  Below, you will be asked for the /usr/man part;
the /manX will be appended automatically, so do not answer /usr/man/man1.

EOF
GMT_man=`get_def_answer "Directory for GMT man pages?" "$GMT_prefix/man"`
GMT_web=`get_def_answer "Directory for GMT www pages?" "$GMT_prefix/www"`

cat << EOF >&2

At run-time GMT will look in the directory $GMT_share to find configuration
and data files.  That directory may appear with a different name to remote users
if a different mount point or a symbolic link is set.
GMT can use the environment variable \$GMT_SHAREDIR to point to the right place.
If users see a different location for the shared data files, specify it here.
(It will be used only to remind you at the end of the installation to set
the enronment variable \$GMT_SHAREDIR).

EOF
GMT_sharedir=`get_def_answer "Enter value of GMT_SHAREDIR selection" "$GMT_share"`

cat << EOF >&2

The answer to the following question will modify the GMT defaults.
(You can always change your mind by editing share/gmt.conf)

EOF
answer=`get_def_answer "Do you prefer SI or US default values for GMT (s/u)" "s"`
if [ "$answer" = "s" ]; then
	GMT_si=y
else
	GMT_si=n
fi

cat << EOF >&2

The answer to the following question will modify the GMT defaults.
(You can always change your mind later by using gmtset)

PostScript (PS) files may contain commands to set paper size, pick
a specific paper tray, or ask for manual feed.  Encapsulated PS
files (EPS) are not intended for printers (but will print ok) and
can be included in other documents.  Both formats will preview
on most previwers (out-of-date Sun pageview is an exception).

EOF
answer=`get_def_answer "Do you prefer PS or EPS as default PostScript output (p/e)" "p"`
if [ "$answer" = "p" ]; then
	GMT_ps=y
else
	GMT_ps=n
fi

cat << EOF >&2

Building the GMT libraries as shared instead of static will
reduce executable sizes considerably.  GMT supports shared
libraries under Linux, Mac OS X, SunOS, Solaris, IRIX, HPUX,
and FreeBSD.  Under other systems you may have to manually
configure macros and determine what specific options to use
with ld.

EOF
GMT_sharedlib=`get_def_answer "Try to make and use shared libraries? (y/n)" "n"`
cat << EOF >&2

If you have more than one C compiler you need to specify which,
otherwise just hit return to use the default compiler.

EOF
GMT_cc=`get_answer "Enter name of C compiler (include path if not in search path)"`

cat << EOF >&2

GMT passes information about previous GMT commands onto later
GMT commands via a hidden file (.gmtcommands).  To avoid that
this file is updated by more than one program at the same time
(e.g., when connecting two or more GMT programs with pipes) we
use POSIX advisory file locking on the file.  Apparently, some
versions of the Network File System (NFS) have not implemented
file locking properly.  We know this is the case with Linux
pre-2.4 kernels when mounting NFS disks from a Unix server.
If this is your case you should turn file locking OFF.

EOF
GMT_flock=`get_def_answer "Use POSIX Advisory File Locking in GMT (y/n)" "y"`

GMT_run_examples=`get_def_answer "Want to test GMT by running the $N_EXAMPLES examples? (y/n)" "y"`
GMT_delete=`get_def_answer "Delete all tar files after install? (y/n)" "n"`

# export CONFIG_SHELL=`type sh | awk '{print $NF}'`

#--------------------------------------------------------------------------------
# Now do coastline archives
#--------------------------------------------------------------------------------

cat << EOF >&2

Normally, all coastline files are installed in $GMT_share/coast.
However, you can also place some of them in separate directories.
These dirs must exist or you must have write permission to make them.
If alternate directories are specified then a coastline.conf file will
be kept in $GMT_share/conf to contain the names of these directories.
NOTE:  Enter full pathname of directory were coastline files are to be stored.

EOF

GMT_dir_cli=`get_def_answer "Directory for int, low, and crude coastline files?" "$GMT_share/coast"`
if [ ! $GMT_get_high = n ]; then
	GMT_dir_high=`get_def_answer "Directory for high coastline files?" "$GMT_dir_cli"`
else
	GMT_dir_high=$GMT_dir_cli
fi
if [ ! $GMT_get_full = n ]; then
	GMT_dir_full=`get_def_answer "Directory for full coastline files?" "$GMT_dir_high"`
else
	GMT_dir_full=$GMT_dir_high
fi

GMT_suppl_dbase=d
GMT_suppl_gshhs=d
GMT_suppl_imgsrc=d
GMT_suppl_meca=d
GMT_suppl_mex=d
GMT_suppl_mgd77=d
GMT_suppl_mgg=d
GMT_suppl_misc=d
GMT_suppl_segyprogs=d
GMT_suppl_spotter=d
GMT_suppl_x2sys=d
GMT_suppl_x_system=d
GMT_suppl_xgrid=d
if [ ! "X$MATLAB" = "X" ]; then
	MATDIR=$MATLAB
elif [ "$os" = "Rhapsody" ] || [ "$os" = "Darwin" ]; then	# Pick one from Applications folder
	(echo /Applications/MATLAB* | grep -v '\*' | head -1 > /tmp/$$.matlab) 2> /dev/null
	if [ ! -s /tmp/$$.matlab ]; then
		MATDIR=`cat /tmp/$$.matlab`
	else
		MATDIR=/usr/local/matlab
	fi
	rm -f /tmp/$$.matlab
else
	MATDIR=/usr/local/matlab
fi

if [ ! $GMT_get_suppl = "n" ]; then

cat << EOF >&2

Several supplemental packages are available:

------------------------------------------------------------------------------
dbase:     Extracting data from NGDC DEM and other grids
gshhs:     Global Self-consistent Hierarchical High-resolution Shoreline extractor
imgsrc:    Extracting grids from global altimeter files (Sandwell/Smith)
meca:      Plotting special symbols in seismology and geodesy
mex:       Matlab interface for reading/writing GMT grdfiles (REQUIRES MATLAB)
mgd77:     Programs for handling MGD77 data files
mgg:       Programs for making, managing, and plotting .gmt files
misc:      Digitize or stitch line segments, read netCDF 1-D tables, and more
segyprogs: Plot SEGY seismic data files
spotter:   Plate tectonic backtracking and hotspotting
x2sys:     New (Generic) Track intersection (crossover) tools
x_system:  Old (MGG-specific) Track intersection (crossover) tools
xgrid:     An X11-based graphical editor for netCDF-based .grd files
------------------------------------------------------------------------------

EOF

	answer=`get_def_answer "Install any of the supplemental programs? (y/n/a(ll))?" "a"`
	if [ "$answer" = "a" ] || [ "$answer" = "n" ]; then
		y_or_n=n
		if [ "$answer" = "a" ]; then
			y_or_n=y
		fi
		GMT_suppl_dbase=$y_or_n
		GMT_suppl_gshhs=$y_or_n
		GMT_suppl_imgsrc=$y_or_n
		GMT_suppl_meca=$y_or_n
		GMT_suppl_mex=$y_or_n
		GMT_suppl_mgd77=$y_or_n
		GMT_suppl_mgg=$y_or_n
		GMT_suppl_misc=$y_or_n
		GMT_suppl_segyprogs=$y_or_n
		GMT_suppl_spotter=$y_or_n
		GMT_suppl_x2sys=$y_or_n
		GMT_suppl_x_system=$y_or_n
		GMT_suppl_xgrid=$y_or_n
	elif [ "$answer" = "y" ]; then
		GMT_suppl_dbase=`get_def_answer "Install the dbase supplemental package? (y/n)?" "y"`
		GMT_suppl_gshhs=`get_def_answer "Install the gshhs supplemental package? (y/n)?" "y"`
		GMT_suppl_imgsrc=`get_def_answer "Install the imgsrc supplemental package? (y/n)?" "y"`
		GMT_suppl_meca=`get_def_answer "Install the meca supplemental package? (y/n)?" "y"`
		GMT_suppl_mex=`get_def_answer "Install the mex supplemental package? (y/n)?" "y"`
		GMT_suppl_mgd77=`get_def_answer "Install the mgd77 supplemental package? (y/n)?" "y"`
		GMT_suppl_mgg=`get_def_answer "Install the mgg supplemental package? (y/n)?" "y"`
		GMT_suppl_misc=`get_def_answer "Install the misc supplemental package? (y/n)?" "y"`
		GMT_suppl_segyprogs=`get_def_answer "Install the segyprogs supplemental package? (y/n)?" "y"`
		GMT_suppl_spotter=`get_def_answer "Install the spotter supplemental package? (y/n)?" "y"`
		GMT_suppl_x2sys=`get_def_answer "Install the x2sys supplemental package? (y/n)?" "y"`
		GMT_suppl_x_system=`get_def_answer "Install the x_system supplemental package? (y/n)?" "y"`
		GMT_suppl_xgrid=`get_def_answer "Install the xgrid supplemental package? (y/n)?" "y"`
	fi
	if [ "$GMT_suppl_mex" = "y" ]; then
		echo " " >&2
		echo "The mex supplement requires Matlab." >&2
		MATDIR=`get_def_answer "Enter MATLAB system directory" "$MATDIR"`
	fi
fi

file=`get_def_answer "Enter name of the parameter file that will now be created" "GMTparam.txt"`

if [ $GMT_FTP_TEST -eq 1 ]; then
	GMT_ftpsite=0
fi

#--------------------------------------------------------------------------------
# SAVE SESSION SETTINGS TO INSTALL.PAR
#--------------------------------------------------------------------------------

cat << EOF > $file
# This file contains parameters needed by the install script
# for GMT Version ${VERSION}.  Give this parameter file
# as the argument to the install_gmt script and the whole
# installation process can be placed in the background.
# Default answers will be selected where none is given.
# You can edit the values, but do not remove definitions!
#
# This script was created by install_gmt on
#
EOF
date | awk '{printf "#\t%s\n", $0}' >> $file
cat << EOF >> $file
#
# Do NOT add any spaces around the = signs.  The
# file MUST conform to Bourne shell syntax
#---------------------------------------------
#       GMT VERSION TO INSTALL
#---------------------------------------------
VERSION=$VERSION
#---------------------------------------------
#       SYSTEM UTILITIES
#---------------------------------------------
GMT_expand=$GMT_expand
GMT_make=$GMT_make
#---------------------------------------------
#       NETCDF SECTION
#---------------------------------------------
netcdf_ftp=$netcdf_ftp
netcdf_install=$netcdf_install
netcdf_path=$netcdf_path
passive_ftp=$passive_ftp
#---------------------------------------------
#       GMT FTP SECTION
#---------------------------------------------
GMT_ftp=$GMT_ftp
GMT_ftpsite=$GMT_ftpsite
GMT_get_src=$GMT_get_src
GMT_get_share=$GMT_get_share
GMT_get_coast=$GMT_get_coast
GMT_get_high=$GMT_get_high
GMT_get_full=$GMT_get_full
GMT_get_suppl=$GMT_get_suppl
GMT_get_scripts=$GMT_get_scripts
GMT_get_pdf=$GMT_get_pdf
GMT_get_man=$GMT_get_man
GMT_get_web=$GMT_get_web
GMT_get_tut=$GMT_get_tut
#---------------------------------------------
#       GMT SUPPLEMENTS SELECT SECTION
#---------------------------------------------
GMT_suppl_dbase=$GMT_suppl_dbase
GMT_suppl_imgsrc=$GMT_suppl_imgsrc
GMT_suppl_gshhs=$GMT_suppl_gshhs
GMT_suppl_meca=$GMT_suppl_meca
GMT_suppl_mex=$GMT_suppl_mex
GMT_suppl_mgd77=$GMT_suppl_mgd77
GMT_suppl_mgg=$GMT_suppl_mgg
GMT_suppl_misc=$GMT_suppl_misc
GMT_suppl_segyprogs=$GMT_suppl_segyprogs
GMT_suppl_spotter=$GMT_suppl_spotter
GMT_suppl_x2sys=$GMT_suppl_x2sys
GMT_suppl_x_system=$GMT_suppl_x_system
GMT_suppl_xgrid=$GMT_suppl_xgrid
#---------------------------------------------
#       GMT ENVIRONMENT SECTION
#---------------------------------------------
GMT_si=$GMT_si
GMT_ps=$GMT_ps
GMT_prefix=$GMT_prefix
GMT_bin=$GMT_bin
GMT_lib=$GMT_lib
GMT_share=$GMT_share
GMT_include=$GMT_include
GMT_man=$GMT_man
GMT_web=$GMT_web
GMT_sharedir=$GMT_sharedir
GMT_dir_full=$GMT_dir_full
GMT_dir_high=$GMT_dir_high
GMT_dir_cli=$GMT_dir_cli
#---------------------------------------------
#       COMPILING & LINKING SECTION
#---------------------------------------------
GMT_sharedlib=$GMT_sharedlib
GMT_cc=$GMT_cc
GMT_triangle=$GMT_triangle
GMT_flock=$GMT_flock
#---------------------------------------------
#       TEST & PRINT SECTION
#---------------------------------------------
GMT_run_examples=$GMT_run_examples
GMT_delete=$GMT_delete
MATDIR=$MATDIR
EOF

echo "Session parameters written to file $file" >&2
echo $file
}
#--------------------------------------------------------------------------------
# BACKGROUND INSTALLATION OF GMT FUNCTIONS
#--------------------------------------------------------------------------------
install_this_gmt()
# Get? File
{
	ok=1
	get_this=$1
	if [ -f GMT${VERSION}_$2.tar.$suffix ]; then
		this=GMT${VERSION}_$2.tar.$suffix
	elif [ -f GMT_$2.tar.$suffix ]; then
		this=GMT_$2.tar.$suffix
	else
		ok=0
	fi
	if [ $ok -eq 1 ] && [ "$get_this" != "n" ]; then	# File exists and we have not said no
		$expand $this | tar xvf -
	fi
}
install_coast()
{
# Get? File dir
	get_this=$1
	file=$2
	dir=$3
	here=`pwd`
	ok=1
	done=0
	if [ -f GMT${VERSION}_${file}.tar.$suffix ]; then
		this=GMT${VERSION}_${file}.tar.$suffix
	elif [ -f GSHHS${GSHHS}_${file}.tar.$suffix ]; then
		this=GSHHS${GSHHS}_${file}.tar.$suffix
	elif [ -f GMT_${file}.tar.$suffix ]; then
		this=GMT_${file}.tar.$suffix
	elif [ -f GSHHS_${file}.tar.$suffix ]; then
		this=GSHHS_${file}.tar.$suffix
	else
		ok=0
	fi
	cd GMT${VERSION}
	if [ $ok -eq 1 ] && [ "$get_this" != "n" ]; then	# File is present and wanted
		if [ ! -d $dir ]; then
			mkdir -p $dir
		fi				
		if [ ! -d $dir ]; then
			echo "Could not make the directory $dir - $this not untarred"
		else
			$expand $here/$this | tar xvf -
			done=1
		fi
	fi
	cd $here
#	Special treatment for Companion CD-ROM with individual bzip2 files for high and full
#	that facilitates cross-platform install with Win32
	if [ -f GMT${file}c.bz2 ] && [ "$get_this" != "n" ] && [ $done -eq 0 ]; then	# File is present and wanted
		if [ ! -d $dir ]; then
			mkdir -p $dir
		fi				
		if [ ! -d $dir ]; then
			echo "Could not make the directory $dir - $this not untarred"
		else
			t=`echo $file | awk '{print substr($1,1,1)}'`
			echo "share/binned_GSHHS_${t}.cdf"
			$expand $here/GMT${file}c.bz2 > $dir/binned_GSHHS_${t}.cdf
			echo "share/binned_binned_${t}.cdf"
			$expand $here/GMT${file}r.bz2 > $dir/binned_river_${t}.cdf
			echo "share/binned_river_${t}.cdf"
			$expand $here/GMT${file}b.bz2 > $dir/binned_border_${t}.cdf
		fi
	fi
		
}

make_ftp_list()
{
# arg1=get arg2=file
	get_this=$1
	file=$2
	if [ "$get_this" = "y" ]; then
		echo "get GMT_${file}.tar.$suffix" >> gmt_install.ftp_list
	fi
}
make_ftp_list2()
{
# arg1=get arg2=file
	get_this=$1
	file=$2
	if [ "$get_this" = "y" ]; then
		echo "get ${file}.tar.$suffix" >> gmt_install.ftp_list
	fi
}
#============================================================
#	START OF MAIN SCRIPT - INITIALIZATION OF PARAMETERS
#============================================================

trap "rm -f gmt_install.ftp_*; exit" 0 2 15
DIR=pub/gmt
#--------------------------------------------------------------------------------
#	LISTING OF CURRENT FTP MIRROR SITES
#--------------------------------------------------------------------------------

N_FTP_SITES=8
cat << EOF > gmt_install.ftp_ip
ftp.soest.hawaii.edu
ibis.grdl.noaa.gov
ftp.iris.washington.edu
ftp.iag.usp.br
ftp.geologi.uio.no
gd.tuwien.ac.at
ftp.scc.u-tokai.ac.jp
life.csu.edu.au
EOF

cat << EOF > gmt_install.ftp_dns
1
1
0
0
0
0
1
0
EOF
#--------------------------------------------------------------------------------

give_help=0
if [ $# -gt 0 ]; then
	if [ "X$1" = "X-h" ] || [ "X$1" = "X-help" ] || [ "X$1" = "X--help" ]; then
		give_help=1
	fi
fi

if [ $give_help -eq 1 ]; then
	cat << EOF >&2
install_gmt - Automatic installation of GMT

GMT is installed in the background following the gathering
of installation parameters.  These parameters are obtained
in one of two ways:

(1) Via Internet: You may compose a parameter file using
    the form on the GMT home page (see the installation
    link under gmt.soest.hawaii.edu) and save the result
    of your request to a parameter file on your hard disk.
2)  Interactively: You may direct this script to start an
    interactive session which gathers information from you
    via a question-and-answer dialog and then saves your
    answers to a parameter file on your hard disk.
    
The parameter file is then passed on to the next stage which
carries out the installation without further interruptions.

Thus, two forms of the command are recognized:

install_gmt [ -c ] parameterfile [ &> logfile]  (for background install)
install_gmt [ -c ] [ -n ] [ &> logfile]	 (for interactive install)

The option -n means do NOT install, just gather the parameters.
The option -c means all tar archices are already in current directory
    and there is no need to ask questions regarding ftp transfers.
Of course, there is also

      install_gmt -h			    (to display this message)
     
EOF
	exit
fi

do_install=1
do_ftp_qa=1
if [ $# -ge 1 ] && [ "$1" = "-n" ]; then	# Do not want to install yet
	do_install=0
	shift
fi
if [ $# -ge 1 ] && [ "$1" = "-c" ]; then	# Local install from cwd - turn off ftp questions
	do_ftp_qa=0
	shift
fi

if [ $# -eq 1 ] && [ "$1" != "-h" ]; then	# User gave a parameter file
	parfile=$1
	if [ ! -f $parfile ]; then
		echo "install_gmt: Parameter file $parfile not found" >&2
		exit
	fi
else			# We must run an interactive session first
	parfile=`prep_gmt`
	if [ $do_install -eq 0 ]; then	# Did not want to install yet
		exit
	fi
fi

#--------------------------------------------------------------------------------
#	INITIATE SETTINGS FROM PARAMETER FILE
#--------------------------------------------------------------------------------
# 
# Because arguments to the . command MUST be in the users PATH
# we must prepend ./ if the file is in the local directory since
# the user may not have . in his PATH
first=`echo $parfile | awk '{print substr($1,1,1)}'`
if [ "$first" = "/" ]; then	# absolute path OK
	. $parfile
else				# Local file, prepend ./
	. ./$parfile
fi

topdir=`pwd`
os=`uname -s`

if [ x"$GMT_expand" = x ]; then	# Was never set by user
	GMT_expand=`which_zip`
fi
if [ "$GMT_expand" = "bzip2" ]; then	# Use bzip2
	suffix="bz2"
	expand="bzip2 -dc"
	echo "+++ Will expand *.bz2 files made with bzip2 +++"
elif [ "$GMT_expand" = "gzip" ]; then
	suffix="gz"
	expand="gzip -dc"
	echo "+++ Will expand *.gz files make with gzip +++" >&2
	echo "    [Consider installing bzip2 (http://sources.redhat.com/bzip2)" >&2
	echo "     since bzip2 files are considerably smaller\!]" >&2
fi

CONFIG_SHELL=`type sh | awk '{print $NF}'`
export CONFIG_SHELL

#--------------------------------------------------------------------------------
#	NETCDF SECTION
#--------------------------------------------------------------------------------

if [ ! x"$GMT_cc" = x ]; then
	CC=$GMT_cc
	export CC
fi

if [ "$netcdf_install" = "y" ]; then
	if [ "$netcdf_ftp" = "y" ]; then

		cd $topdir
	
#		Determine if client's ftp is set to active or passive mode by default

		echo passive | \ftp | grep -q off
		active=$?

#		Set-up ftp command

		echo "user anonymous $USER@" > $$
		if [ "$passive_ftp" = "y" ] && [ $active -eq 1 ]; then
			echo "passive" >> $$
			echo "quote pasv" >> $$
		fi
		echo "cd pub/netcdf" >> $$
		echo "binary" >> $$
		echo "get netcdf.tar.Z" >> $$
		echo "quit" >> $$
		echo " " >> $$

#		Get the file

		echo "Getting netcdf by anonymous ftp (be patient)..." >&2
		before=`du -sk . | cut -f1`
		ftp -dn ftp.unidata.ucar.edu < $$ || ( echo "ftp failed - try again later" >&2; exit )
		after=`du -sk . | cut -f1`
		newstuff=`echo $before $after | awk '{print $2 - $1}'`
		echo "Got $newstuff kb ... done" >&2
		rm -f $$
	fi

	if [ -f netcdf.tar.Z ]; then
		zcat netcdf.tar.Z | tar xvf -
	elif [ -f netcdf.tar.bz2 ] && [ "$GMT_expand" = "bzip2" ]; then
		$expand netcdf.tar.$suffix | tar xvf -
	elif [ -f netcdf.tar.gz ] && [ "$GMT_expand" = "gzip" ]; then
		$expand netcdf.tar.$suffix | tar xvf -
	else
		echo "?? netcdf.tar.{Z,bz2,gz} not found - must abort !!"
		exit
	fi
	
	cd netcdf-${NETCDF_VERSION}

	if [ "$os" = "Interix" ]; then	# Windows SFU
		CC=${CC=gcc}
	fi
	netcdf_path=${netcdf_path:-$topdir/netcdf-${NETCDF_VERSION}}
	rm -f config.{cache,log,status}
	./configure --prefix=$netcdf_path --enable-c-only --enable-shared
	$GMT_make check   || exit
	$GMT_make install || exit
	$GMT_make clean   || exit
	cd $topdir
	if [ "$GMT_delete" = "y" ]; then
		rm -f netcdf*.tar.Z
	fi
fi

if [ x"$netcdf_path" = x ]; then	# Not explicitly set, must assign it
	if [ ! x"$NETCDFHOME" = x ]; then	# Good, used an environmental variable for it
                netcdf_path=$NETCDFHOME
        elif [ "$netcdf_ftp" = "n" ]; then	# Next, see if it was already installed in $topdir
 		netcdf_path=$topdir/netcdf-${NETCDF_VERSION}
		if [ -d $netcdf_path ]; then	# OK, it was there
			p=	# Dummy for empty branch
		elif [ -d /usr/local/netcdf/lib ]; then	# No, try some standard places
      			netcdf_path="/usr/local/netcdf"
		elif [ -f /sw/lib/libnetcdf.a ]; then	# Mac OSX with fink
      			netcdf_path="/sw"
		else
               		echo "install_gmt: No path for netcdf provided - abort" >&2
               		echo "install_gmt: netcdf not in /usr/local, /usr/local/netcdf, or /sw" >&2
			exit
		fi
        fi
	echo "install_gmt: netcdf found in $netcdf_path" >&2
fi
NETCDFHOME=$netcdf_path
export NETCDFHOME

#--------------------------------------------------------------------------------
#	GMT FTP SECTION
#--------------------------------------------------------------------------------

cd $topdir
if [ "$GMT_ftp" = "y" ]; then

	if [ $GMT_ftpsite -eq 0 ]; then
		GMT_ftpsite=1
		echo " [Special GMT guru ftp site selected]" >&2
		GMT_FTP_TEST=1
	fi
	if [ $GMT_ftpsite -lt 0 ] || [ $GMT_ftpsite -gt $N_FTP_SITES ]; then
		GMT_ftpsite=1
		echo " Error in assigning site, use default site $GMT_ftpsite" >&2
	fi
	if [ $GMT_ftpsite -eq 1 ]; then	# SOEST's server starts at / and there is no pub
		if [ $GMT_FTP_TEST -eq 1 ]; then	# Special dir for guru testing
			DIR=gmttest
		else
			DIR=gmt
		fi
	fi
	ftp_ip=`sed -n ${GMT_ftpsite}p gmt_install.ftp_ip`
	is_dns=`sed -n ${GMT_ftpsite}p gmt_install.ftp_dns`

#	Determine if client's ftp is set to active or passive mode by default

	echo passive | \ftp | grep -q off
	active=$?

#	Set-up ftp command
	sub=`echo $VERSION | awk '{print substr($1,1,1)}'`
	echo "user anonymous $USER@" > gmt_install.ftp_list
	if [ "$passive_ftp" = "y" ] && [ $active -eq 1 ]; then
		echo "passive" >> gmt_install.ftp_list
		echo "quote pasv" >> gmt_install.ftp_list
	fi
	echo "cd $DIR/$sub" >> gmt_install.ftp_list
	echo "binary" >> gmt_install.ftp_list
	make_ftp_list $GMT_get_src src
	make_ftp_list $GMT_get_share share
	make_ftp_list $GMT_get_coast coast
	make_ftp_list $GMT_get_high high
	make_ftp_list $GMT_get_full full
	make_ftp_list $GMT_get_suppl suppl
	make_ftp_list $GMT_get_scripts scripts
	make_ftp_list $GMT_get_pdf pdf
	make_ftp_list $GMT_get_man man
	make_ftp_list $GMT_get_web web
	make_ftp_list $GMT_get_tut tut
	echo "quit" >> gmt_install.ftp_list
	echo " " >> gmt_install.ftp_list

#	Get the files

	echo "Getting GMT by anonymous ftp from $ftp_ip (be patient)..." >&2

	before=`du -sk . | cut -f1`
	ftp -dn $ftp_ip < gmt_install.ftp_list || ( echo "fpt failed - try again later >&2"; exit )
	after=`du -sk . | cut -f1`
	rm -f gmt_install.ftp_list
	newstuff=`echo $before $after | awk '{print $2 - $1}'`
	echo "Got $newstuff kb ... done" >&2
fi

# If we got here via a parameter file that had blank answers
# we need to provide the default values here

GMT_prefix=${GMT_prefix:-$topdir/GMT${VERSION}}
GMT_bin=${GMT_bin:-$GMT_prefix/bin}
GMT_lib=${GMT_lib:-$GMT_prefix/lib}
GMT_share=${GMT_share:-$GMT_prefix/share}
GMT_include=${GMT_include:-$GMT_prefix/include}
GMT_man=${GMT_man:-$GMT_prefix/man}
GMT_web=${GMT_web:-$GMT_prefix/www}
GMT_sharedir=${GMT_sharedir:-$GMT_share}

#--------------------------------------------------------------------------------
# First install source code and documentation
#--------------------------------------------------------------------------------

install_this_gmt $GMT_get_src src
install_this_gmt $GMT_get_share share
install_this_gmt $GMT_get_suppl suppl
install_this_gmt $GMT_get_scripts scripts
install_this_gmt $GMT_get_pdf pdf
install_this_gmt $GMT_get_man man
install_this_gmt $GMT_get_web web
install_this_gmt $GMT_get_tut tut

#--------------------------------------------------------------------------------
# Now do coastline archives
#--------------------------------------------------------------------------------

dir=$GMT_share/coast
GMT_dir_full=${GMT_dir_full:-$dir}
if [ "$GMT_dir_full" != "$dir" ]; then
	echo $GMT_dir_full >> $$.coast
fi

GMT_dir_high=${GMT_dir_high:-$dir}
if [ "$GMT_dir_high" != "$dir" ]; then
	echo $GMT_dir_high >> $$.coast
fi

GMT_dir_cli=${GMT_dir_cli:-$dir}
if [ "$GMT_dir_cli" != "$dir" ]; then
	echo $GMT_dir_cli >> $$.coast
fi

install_coast $GMT_get_coast coast $GMT_dir_cli
install_coast $GMT_get_high  high  $GMT_dir_high
install_coast $GMT_get_full  full  $GMT_dir_full

if [ -f $$.coast ]; then	# Install coastline.conf file
	echo "# GMT Coastline Path Configuration File" > $topdir/GMT${VERSION}/share/conf/coastline.conf
	sort -u $$.coast >> $topdir/GMT${VERSION}/share/conf/coastline.conf
	echo "$topdir/GMT${VERSION}/share/conf/coastline.conf initialized" >&2
	rm -f $$.coast
fi

echo " " >&2
echo "write privileges on all files in GMT${VERSION} ..." >&2
cd GMT${VERSION}
chmod -R +w .
cd ..
echo "Done" >&2
echo " " >&2

#--------------------------------------------------------------------------------
#	GMT INSTALLATION PREPARATIONS
#--------------------------------------------------------------------------------

cd $topdir
cd GMT${VERSION}
here=`pwd`

# Are we allowed to write in $GMT_share?

mkdir -p $GMT_share
if [ -w $GMT_share ]; then
	write_share=1
else
	write_share=0
fi

# Are we allowed to write in $GMT_bin?

mkdir -p $GMT_bin
if [ -w $GMT_bin ]; then
	write_bin=1
else
	write_bin=0
fi

# Are we allowed to write in $GMT_man?

mkdir -p $GMT_man
if [ -w $GMT_man ]; then
	write_man=1
else
	write_man=0
fi

# Are we allowed to write in $GMT_web?

mkdir -p $GMT_web
if [ -w $GMT_web ]; then
	write_web=1
else
	write_web=0
fi

#--------------------------------------------------------------------------------
#	CONFIGURE PREPARATION
#--------------------------------------------------------------------------------

if [ "$GMT_si" = "y" ]; then
	enable_us=
else
	enable_us=--enable-US
fi
if [ "$GMT_ps" = "y" ]; then
	enable_eps=
else
	enable_eps=--enable-eps
fi
if [ "$GMT_flock" = "y" ]; then
	disable_flock=
else
	disable_flock=--disable-flock
fi
if [ "$GMT_triangle" = "y" ]; then
	enable_triangle=--enable-triangle
else
	enable_triangle=
fi
if [ "$GMT_suppl_mex" = "y" ]; then
	disable_mex=
else
	disable_mex=--disable-mex
fi
if [ "$GMT_suppl_xgrid" = "y" ]; then
	disable_xgrid=
else
	disable_xgrid=--disable-xgrid
fi

if [ "$GMT_sharedlib" = "y" ]; then
	enable_shared=--enable-shared
else
	enable_shared=
fi

if [ "$GMT_64" = "y" ]; then
	enable_64=--enable-64
else
	enable_64=
fi

if [ ! x"$MATDIR" = x ]; then	# MATDIR is set
	enable_matlab=--enable-matlab=$MATDIR
else
	enable_matlab=
fi

#--------------------------------------------------------------------------------
#	GMT installation commences here
#--------------------------------------------------------------------------------

cat << EOF >&2

---> Begin GMT $VERSION installation <---

---> Run configure to create makegmt.macros and gmt_notposix.h

EOF

# Clean out old cached values

rm -f config.{cache,log,status}

# Clean out old makegmt.macros etc if present

if [ -f src/makegmt.macros ]; then
	echo '---> Clean out old executables, *.o, *.a, and makegmt.macros' >&2
	$GMT_make spotless || exit
fi
	
./configure --prefix=$GMT_prefix --bindir=$GMT_bin --libdir=$GMT_lib --includedir=$GMT_include $enable_us \
  --enable-netcdf=$netcdf_path $enable_matlab $enable_eps $disable_flock $enable_shared $enable_triangle $enable_64 \
  --mandir=$GMT_man --enable-www=$GMT_web --datadir=$GMT_share --enable-update=$ftp_ip \
  $disable_mex $disable_xgrid 

if [ -f .gmtconfigure ]; then
	cat .gmtconfigure
	rm -f .gmtconfigure
fi

# OK, descend into src directory

cd src

echo "---> Make all" >&2

$GMT_make all || exit

if [ $write_bin -eq 1 ]; then
	echo "---> Make install" >&2

	$GMT_make install || exit
else
	echo "You do not have write permission to make $GMT_bin" >&2
fi

cd ..

#--------------------------------------------------------------------------------
# INSTALL SUPPLEMENTAL PROGRAMS
#--------------------------------------------------------------------------------

if [ -d src/dbase ]; then
	echo "---> Make Supplements" >&2
	$GMT_make suppl || exit
	if [ $write_bin -eq 1 ]; then
		echo "---> Make install Supplements" >&2

		$GMT_make install || exit
	else
		echo "You do not have write permission to make $GMT_bin" >&2
	fi
fi

#--------------------------------------------------------------------------------
# INSTALL LIB DIRECTORY
#--------------------------------------------------------------------------------

if [ $write_share -eq 1 ]; then
	$GMT_make install-data || exit
fi

#--------------------------------------------------------------------------------
# INSTALL MAN PAGES
#--------------------------------------------------------------------------------

if [ $write_man -eq 1 ]; then
	$GMT_make install-man || exit
	echo "All users must include $GMT_man in their MANPATH" >&2
else
	echo "You do not have write permission to make $GMT_man" >&2
fi


#--------------------------------------------------------------------------------
# INSTALL WWW PAGES
#--------------------------------------------------------------------------------

if [ $write_web -eq 1 ]; then
	if [ -d www ]; then
		$GMT_make install-www || exit
		echo "All users should add $GMT_web/gmt/gmt_services.html to their browser bookmarks" >&2
	fi
else
	echo "You do not have write permission to create $GMT_web" >&2
fi

#--------------------------------------------------------------------------------
# RUN EXAMPLES
#--------------------------------------------------------------------------------

# Run examples with /src as binary path in case the user did
# not have permission to place files in GMT_bin

if [ -d examples ] && [ "$GMT_run_examples" = "y" ]; then
	GMT_SHAREDIR=$GMT_sharedir
	export GMT_SHAREDIR
	$GMT_make run-examples || exit
fi


cd $here/src
if [ $write_bin -eq 1 ]; then
	$GMT_make clean || ( echo "Problems during make clean - check manually" >&2 )
else
	echo "Manually do the final installs as another user (root?)" >&2
	echo "Go to the main GMT directory and say:" >&2
	echo "make install install-suppl clean" >&2
fi
if [ $write_share -eq 0 ]; then
	echo "Manually do the coastline install as another user (root?)" >&2
	echo "Go to the main GMT directory and say:" >&2
	echo "make install-data" >&2
fi
if [ $write_man -eq 0 ]; then
	echo "Manually do the man page install as another user (root?)" >&2
	echo "Go to the main GMT directory and say:" >&2
	echo "make install-man" >&2
fi
if [ $write_web -eq 0 ]; then
	echo "Manually do the www page install as another user (root?)" >&2
	echo "Go to the main GMT directory and say:" >&2
	echo "make install-www" >&2
fi

cd $topdir
if [ "$GMT_delete" = "y" ]; then
	rm -f GMT*.tar.$suffix
	if [ -f GMTfullc.bz2 ]; then	# Special files copied from CD-ROM
		rm -f GMTfull?.bz2 GMThigh?.bz2
	fi
fi

cat << EOF >&2
GMT installation complete. Remember to set these:

-----------------------------------------------------------------------
For csh or tcsh users:
setenv NETCDFHOME $NETCDFHOME
set path=($GMT_bin \$path)
EOF
if [ "$GMT_sharedir" != "$GMT_share" ]; then
	echo setenv GMT_SHAREDIR $GMT_sharedir >&2
fi
cat << EOF >&2

For sh or bash users:
export NETCDFHOME=$NETCDFHOME
export PATH=$GMT_bin:\$PATH
EOF
if [ "$GMT_sharedir" != "$GMT_share" ]; then
	echo export GMT_SHAREDIR=$GMT_sharedir >&2
fi
cat << EOF >&2

For all users:
EOF
if [ ! x"$GMT_man" = x ]; then
	echo "Add $GMT_man to MANPATH" >&2
fi
if [ ! x"$GMT_web" = x ]; then
	echo "Add $GMT_web/gmt/gmt_services.html as browser bookmark" >&2
fi
echo "-----------------------------------------------------------------------" >&2
rm -f gmt_install.ftp_*
