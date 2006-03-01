#!/bin/sh
#-----------------------------------------------------------------------------
#	 $Id: pdfman.sh,v 1.8 2006-03-01 03:50:16 pwessel Exp $
#
#	pdfman.sh - Automatic generation of the GMT pdf manual pages
#
#	Author:	Paul Wessel
#	Date:	1-MAR-2006
#	Version: 1.1 Bourne shell
#
#	Uses groff -man
#	Assumes a cvs update has occured so files are fresh.
#
#	Must be run from the main GMT directory after man pages have
#	been made.
#
#-----------------------------------------------------------------------------

trap 'rm -f $$.*; exit 1' 1 2 3 15

if [ $#argv = 1 ]; then	# If -s is given we run silently with defaults
	gush=0
else			# else we make alot of noise
	gush=1
fi

mkdir -p www/gmt/doc/ps
mkdir -p www/gmt/doc/pdf

# First make a list of all the GMT programs, including pslib (since it has a man page) and the GMT script

grep -v '^#' guru/GMT_programs.lis > $$.programs.lis
echo GMT >> $$.programs.lis
echo pslib >> $$.programs.lis

# Ok, make pdf files
if [ $gush = 1 ]; then
	echo "Assembling GMT_Manpages.ps"
fi
add=0
rm -f www/gmt/doc/ps/GMT_Manpages.ps www/gmt/doc/pdf/GMT_Manpages.pdf
for prog in `cat $$.programs.lis`; do
	if [ $gush = 1 ]; then
		echo "Appending ${prog}.pdf"
	fi
	if [ $add -eq 1 ]; then
		echo "false 0 startjob pop" >> www/gmt/doc/ps/GMT_Manpages.ps
	fi
	add=1
	groff -man man/manl/${prog}.l >> www/gmt/doc/ps/GMT_Manpages.ps
done

# Ok, then do the supplemental packages

# Gurus who have their own supplemental packages can have them processed too by
# defining an environmental parameter MY_GMT_SUPPL which contains a list of these
# supplements.  They must all be in src of course

if [ $gush = 1 ]; then
	echo "Assembling GMT_Manpages_suppl.ps"
fi
MY_SUPPL=${MY_GMT_SUPPL:-""}
cd src
add=0
rm -f www/gmt/doc/ps/GMT_Manpages_suppl.ps www/gmt/doc/pdf/GMT_Manpages_suppl.pdf
for package in dbase imgsrc meca mgd77 mgg misc segyprogs spotter x2sys x_system $MY_SUPPL; do
	for f in $package/*.man; do
		prog=`basename $f .man`
		if [ $gush = 1 ] && [ -f ../man/manl/$prog.l ]; then
			echo "Appending ${prog}.pdf"
			if [ $add -eq 1 ]; then
				echo "false 0 startjob pop" >> ../www/gmt/doc/ps/GMT_Manpages_suppl.ps
			fi
			add=1
			groff -man ../man/manl/$prog.l >> ../www/gmt/doc/ps/GMT_Manpages_suppl.ps
		fi
	done
done
cd ..

# Convert to PDF

echo "Converting GMT_Manpages.ps to GMT_Manpages.pdf"
ps2pdf www/gmt/doc/ps/GMT_Manpages.ps www/gmt/doc/pdf/GMT_Manpages.pdf
echo "Converting GMT_Manpages_suppl.ps to GMT_Manpages_suppl.pdf"
ps2pdf www/gmt/doc/ps/GMT_Manpages_suppl.ps www/gmt/doc/pdf/GMT_Manpages_suppl.pdf
