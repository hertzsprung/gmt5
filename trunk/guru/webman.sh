#!/bin/sh
#-----------------------------------------------------------------------------
#	 $Id: webman.sh,v 1.41 2006-10-26 16:28:00 remko Exp $
#
#	webman.sh - Automatic generation of the GMT web manual pages
#
#	Author:	Paul Wessel
#	Date:	17-SEP-1999
#	Version: 1.2 Bourne shell
#
#	Uses groff -T html + alot of sed and awk...
#	Assumes a cvs update has occured so files are fresh.
#
#	Must be run from the main GMT directory after man pages have
#	been made.
#
#-----------------------------------------------------------------------------

trap 'rm -f $$.*; exit 1' 1 2 3 15

if [ $# = 1 ]; then	# If -s is given we run silently with defaults
	gush=0
else			# else we make alot of noise
	gush=1
fi

mkdir -p www/gmt/doc/html

# First make a list of all the GMT programs, including pslib (since it has a man page) and the GMT script

grep -v '^#' guru/GMT_programs.lis > $$.programs.lis
echo GMT >> $$.programs.lis
echo pslib >> $$.programs.lis

# Then add the supplemental programs as well
grep 'html$' guru/GMT_suppl.lis | sed -e 's/\.html$//g' | awk -F/ '{print $NF}' >> $$.programs.lis

# Now make sed script that will replace the bold and italic versions of GMT program names with
# similarly formatted links.

awk '{printf "s%%<b>%s</b>%%<A HREF=%c%s.html%c><b>%s</b></A>%%g\n", $1, 34, $1, 34, $1}' $$.programs.lis > $$.w0.sed
awk '{printf "s%%<i>%s</i>%%<A HREF=%c%s.html%c><i>%s</i></A>%%g\n", $1, 34, $1, 34, $1}' $$.programs.lis >> $$.w0.sed

# Make sed script that adds active links to gmtdefault for all GMT defaults parameters.
# all.sed is run on all programs except gmtdefaults and add links to the relevant anchors in gmtdefaults.html
# def.sed adds the anchors needed in gmtdefaults.html

grep -v '^#' src/gmt_keywords.d | awk '{printf "s%%<b>%s</b>%%<A HREF=%cgmtdefaults.html#%s%c><b>%s</b></A>%%g\n", $1, 34, $1, 34, $1}' > $$.all.sed
grep -v '^#' src/gmt_keywords.d | awk '{printf "s%%<p><b>%s</b></p></td>%%<A NAME=%c%s%c><p><b>%s</b></p></td>%%g\n", $1, 34, $1, 34, $1}' > $$.def.sed

# Ok, go to source directory and make html files
for prog in `cat $$.programs.lis`; do
	[ $gush = 1 ] && echo "Making ${prog}.html"
	# Remove reference to current programs since no program needs active links to itself
	grep -v "${prog}<" $$.w0.sed > $$.t0.sed
	groff -man -T html man/manl/${prog}.l | sed -f $$.t0.sed > $$.tmp
	if [ "X$prog" = "Xgmtdefaults" ]; then
		sed -f $$.def.sed -f $$.all.sed $$.tmp > www/gmt/doc/html/${prog}.html
	else
		sed -f $$.all.sed $$.tmp > www/gmt/doc/html/${prog}.html
	fi
	echo '<BODY bgcolor="#ffffff">' >> www/gmt/doc/html/${prog}.html
done

# Ok, then do the supplemental packages

# Gurus who have their own supplemental packages can have them processed too by
# defining an environmental parameter MY_GMT_SUPPL which contains a list of these
# supplements.  They must all be in src of course

MY_SUPPL=${MY_GMT_SUPPL:-""}
cd src
for package in dbase imgsrc meca mgd77 mgg misc segyprogs spotter x2sys x_system $MY_SUPPL; do
	ls $package/*.man > ../$$.lis
	while read f; do
		prog=`basename $f .man`
		if [ -f ../man/manl/$prog.l ]; then
			[ $gush = 1 ] && echo "Making ${prog}.html"
			# Remove reference to current programs since no program needs active links to itself
			grep -v "${prog}<" ../$$.w0.sed > ../$$.t0.sed
			groff -man -T html ../man/manl/$prog.l | sed -f ../$$.t0.sed | sed -f ../$$.all.sed > $package/${prog}.html
			echo '<BODY bgcolor="#ffffff">' >> $package/${prog}.html
			cp -f $package/${prog}.html ../www/gmt/doc/html
		fi
	done < ../$$.lis
done
cd ..

#-----------------------------------------------------------------
#	Generate the main GMT Manual page directory
#-----------------------------------------------------------------

cat << EOF > www/gmt/gmt_man.html
<HTML>
<!-- gmt_man.html - Automatically generated by webman.sh  -->
<TITLE>GMT Online Man Pages</TITLE>
<BODY bgcolor="#ffffff">
<CENTER><H2>GMT Online Man Pages</H2></CENTER><P>
These man pages are html-versions of the Unix man pages for GMT.  They
are grouped both alphabetically or by function.<BR><HR>
<H3><A NAME="anchor_theme">Thematic listing of GMT Unix man pages</H3>
For alphabetical order, click <A HREF="#anchor_alpha">here</A>.
<H3>GMT WRAPPER:</H3>
<UL>
<LI><A HREF="doc/html/GMT.html"> GMT</A> A Wrapper for GMT programs
</UL>
<H3>FILTERING OF 1-D AND 2-D DATA:</H3>
<UL>
<LI><A HREF="doc/html/blockmean.html"> blockmean</A> L2 (x,y,z) data filter/decimator
<LI><A HREF="doc/html/blockmedian.html"> blockmedian</A> L1 (x,y,z) data filter/decimator
<LI><A HREF="doc/html/blockmode.html"> blockmode</A> Mode-estimating (x,y,z) data filter/decimator
<LI><A HREF="doc/html/filter1d.html"> filter1d</A> Filter 1-D data (time series)
<LI><A HREF="doc/html/grdfilter.html"> grdfilter</A> Filter 2-D data in space domain
</UL>
<H3>PLOTTING OF 1-D and 2-D DATA:</H3>
<UL>
<LI><A HREF="doc/html/grdcontour.html"> grdcontour</A> Contouring of 2-D gridded data
<LI><A HREF="doc/html/grdimage.html"> grdimage</A> Produce images from 2-D gridded datar
<LI><A HREF="doc/html/grdvector.html"> grdvector</A> Plot vector fields from 2-D gridded data
<LI><A HREF="doc/html/grdview.html"> grdview</A> 3-D perspective imaging of 2-D gridded data
<LI><A HREF="doc/html/psbasemap.html"> psbasemap</A> Create a basemap frame
<LI><A HREF="doc/html/psclip.html"> psclip</A> Use polygon files as clipping paths
<LI><A HREF="doc/html/pscoast.html"> pscoast</A> Plot coastlines, filled continents, rivers, and political borders
<LI><A HREF="doc/html/pscontour.html"> pscontour</A> Direct contouring or imaging of xyz-data by triangulation
<LI><A HREF="doc/html/pshistogram.html"> pshistogram</A> Plot a histogram
<LI><A HREF="doc/html/psimage.html"> psimage</A> Plot Sun rasterfiles on a map
<LI><A HREF="doc/html/pslegend.html"> pslegend</A> Plot legend on a map
<LI><A HREF="doc/html/psmask.html"> psmask</A> Create overlay to mask specified regions of a map
<LI><A HREF="doc/html/psrose.html"> psrose</A> Plot sector or rose diagrams
<LI><A HREF="doc/html/psscale.html"> psscale</A> Plot grayscale or colorscale
<LI><A HREF="doc/html/pstext.html"> pstext</A> Plot textstrings
<LI><A HREF="doc/html/pswiggle.html"> pswiggle</A> Draw anomalies along track
<LI><A HREF="doc/html/psxy.html"> psxy</A> Plot symbols, polygons, and lines in 2-D
<LI><A HREF="doc/html/psxyz.html"> psxyz</A> Plot symbols, polygons, and lines in 3-D
</UL>
<H3>GRIDDING OF (X,Y,Z) DATA:</H3>
<UL>
<LI><A HREF="doc/html/nearneighbor.html"> nearneighbor</A> Nearest-neighbor gridding scheme
<LI><A HREF="doc/html/surface.html"> surface</A> Continuous curvature gridding algorithm
<LI><A HREF="doc/html/triangulate.html"> triangulate</A> Perform optimal Delauney triangulation on xyz data
</UL>
<H3>SAMPLING OF 1-D AND 2-D DATA:</H3>
<UL>
<LI><A HREF="doc/html/grdsample.html"> grdsample</A> Resample a 2-D gridded data onto new grid
<LI><A HREF="doc/html/grdtrack.html"> grdtrack</A> Sampling of 2-D data along 1-D track
<LI><A HREF="doc/html/sample1d.html"> sample1d</A> Resampling of 1-D data
</UL>
<H3>PROJECTION AND MAP-TRANSFORMATION:</H3>
<UL>
<LI><A HREF="doc/html/grdproject.html"> grdproject</A> Project gridded data onto new coordinate system
<LI><A HREF="doc/html/mapproject.html"> mapproject</A> Transformation of coordinate systems
<LI><A HREF="doc/html/project.html"> project</A> Project data onto lines/great circles
</UL>
<H3>INFORMATION:</H3>
<UL>
<LI><A HREF="doc/html/gmtdefaults.html"> gmtdefaults</A> List the current default settings
<LI><A HREF="doc/html/gmtset.html"> gmtset</A> Edit parameters in the .gmtdefaults file
<LI><A HREF="doc/html/grdinfo.html"> grdinfo</A> Get information about grd files
<LI><A HREF="doc/html/minmax.html"> minmax</A> Report extreme values in table datafiles
</UL>
<H3>CONVERT OR EXTRACT SUBSETS OF DATA:</H3>
<UL>
<LI><A HREF="doc/html/gmt2rgb.html"> gmt2rgb</A> Convert Sun raster or grdfile to red, green, blue component grids
<LI><A HREF="doc/html/gmtconvert.html"> gmtconvert</A> Convert table data from one format to another
<LI><A HREF="doc/html/gmtmath.html"> gmtmath</A> Reverse Polish calculator for table data
<LI><A HREF="doc/html/gmtselect.html"> gmtselect</A> Select table subsets based on multiple spatial criteria
<LI><A HREF="doc/html/grd2xyz.html"> grd2xyz</A> Convert 2-D gridded data to table
<LI><A HREF="doc/html/grdcut.html"> grdcut</A> Cut a sub-region from a grd file
<LI><A HREF="doc/html/grdpaste.html"> grdpaste</A> Paste together grdfiles along common edge
<LI><A HREF="doc/html/grdreformat.html"> grdreformat</A> Convert from one grdformat to another
<LI><A HREF="doc/html/splitxyz.html"> splitxyz</A> Split xyz files into several segments
<LI><A HREF="doc/html/xyz2grd.html"> xyz2grd</A> Convert table to 2-D grd file
</UL>
<H3>MISCELLANEOUS:</H3>
<UL>
<LI><A HREF="doc/html/makecpt.html"> makecpt</A> Create GMT color palette tables
<LI><A HREF="doc/html/spectrum1d.html"> spectrum1d</A> Compute spectral estimates from time-series
<LI><A HREF="doc/html/triangulate.html"> triangulate</A> Perform optimal Delauney triangulation on xyz data
</UL>
<H3>DETERMINE TRENDS IN 1-D AND 2-D DATA:</H3>
<UL>
<LI><A HREF="doc/html/fitcircle.html"> fitcircle</A> Finds best-fitting great or small circles
<LI><A HREF="doc/html/grdtrend.html"> grdtrend</A> Fits polynomial trends to grdfiles (z = f(x,y))
<LI><A HREF="doc/html/trend1d.html"> trend1d</A> Fits polynomial or Fourier trends to y = f(x) series
<LI><A HREF="doc/html/trend2d.html"> trend2d</A> Fits polynomial trends to z = f(x,y) series
</UL>
<H3>OTHER OPERATIONS ON 2-D GRIDS:</H3>
<UL>
<LI><A HREF="doc/html/grd2cpt.html"> grd2cpt</A> Make color palette table from grdfile
<LI><A HREF="doc/html/grdblend.html"> grdblend</A> Blend several gridded data sets into one
<LI><A HREF="doc/html/grdclip.html"> grdclip</A> Limit the z-range in gridded data sets
<LI><A HREF="doc/html/grdedit.html"> grdedit</A> Modify grd header information
<LI><A HREF="doc/html/grdfft.html"> grdfft</A> Operate on grdfiles in frequency domain
<LI><A HREF="doc/html/grdgradient.html"> grdgradient</A> Compute directional gradient from grdfiles
<LI><A HREF="doc/html/grdhisteq.html"> grdhisteq</A> Histogram equalization for grdfiles
<LI><A HREF="doc/html/grdlandmask.html"> grdlandmask</A> Creates mask grdfile from coastline database
<LI><A HREF="doc/html/grdmask.html"> grdmask</A> Set nodes outside a clip path to a constant
<LI><A HREF="doc/html/grdmath.html"> grdmath</A> Reverse Polish calculator for grdfiles
<LI><A HREF="doc/html/grdvolume.html"> grdvolume</A> Calculating volume under a surface within a contour
</UL>
<H3>POSTSCRIPT LIBRARY FUNCTION CALLS (for developers):</H3>
<UL>
<LI><A HREF="doc/html/pslib.html"> pslib</A> Reference manual for libpsl.a
</UL>
<HR>
<H3><A NAME="anchor_alpha">Alphabetical listing of GMT Unix man pages</H3>
For a thematic listing, click <A HREF="#anchor_theme">here</A>.
<OL>
EOF
# Exclude pslib since it is not a program
grep -v pslib $$.programs.lis | awk '{printf "<LI><A HREF=%cdoc/html/%s.html%c> %s</A>\n", 34, $1, 34, $1}' >> www/gmt/gmt_man.html
cat << EOF >> www/gmt/gmt_man.html
</OL>
<HR>
For a thematic listing of GMT man pages, click <A HREF="#anchor_theme">here</A>.
<A HREF="gmt_services.html">
<IMG SRC="gmt_back.gif" ALT="RETURN">
Return to GMT Online Services page.
</A>
</BODY>
</HTML>
EOF

#-----------------------------------------------------------------
#	Generate the supplemental GMT Manual page directory
#-----------------------------------------------------------------

cat << EOF > www/gmt/gmt_suppl.html
<HTML>
<!-- gmt_suppl.html - Automatically generated by webman.sh  -->
<TITLE>GMT Supplemental Online Man Pages</TITLE>
<BODY bgcolor="#ffffff">
<CENTER><H2>GMT Supplemental Online Man Pages</h2></CENTER><P>
These man pages are html-versions of the Unix man pages for the GMT
supplemental programs, grouped by package.  Note that only the
packages actually installed on your system will be accessible.
<HR>
<H3>The DBASE package</H3>
<UL>
<LI><A HREF="doc/html/grdraster.html"> grdraster</A> Extraction of gridfiles from databases
</UL>
<HR>
<H3>The IMGSRC package</H3>
<UL>
<LI><A HREF="doc/html/img2grd.html"> img2grd</A> Front-end Bourne script to img2mercgrd
<LI><A HREF="doc/html/img2mercgrd.html"> img2mercgrd</A> Extracting data from Sandwell/Smith altimetry solutions
</UL>
<HR>
<H3>The MECA package</H3>
<UL>
<LI><A HREF="doc/html/pscoupe.html"> pscoupe</A> Plot seismic moment tensor and/or double couples on a cross-section
<LI><A HREF="doc/html/psmeca.html"> psmeca</A> Plot seismic moment tensor and/or double couples on maps
<LI><A HREF="doc/html/pspolar.html"> pspolar</A> Plot polarities on the lower half-sphere
<LI><A HREF="doc/html/psvelo.html"> psvelo</A> Plot velocity ellipses, strain crosses, or strain wedges on maps
</UL>
<HR>
<H3>The MGD77 package</H3>
<UL>
<LI><A HREF="doc/html/mgd77convert.html"> mgd77convert</A> Convert between different MGD77 file formats
<LI><A HREF="doc/html/mgd77info.html"> mgd77info</A> Obtain information from MGD77 cruise files
<LI><A HREF="doc/html/mgd77list.html"> mgd77list</A> Extract data from MGD77 cruise files
<LI><A HREF="doc/html/mgd77manage.html"> mgd77manage</A> Manage the enhanced MGD77+ netCDF files
<LI><A HREF="doc/html/mgd77path.html"> mgd77path</A> Get full pathnames of MGD77 cruise files
<LI><A HREF="doc/html/mgd77sniffer.html"> mgd77sniffer</A> Along-track quality control of MGD77 cruise files
<LI><A HREF="doc/html/mgd77track.html"> mgd77track</A> Plot the tracks of MGD77 cruise files
</UL>
<HR>
<H3>The MGG package</H3>
<UL>
<LI><A HREF="doc/html/binlegs.html"> binlegs</A> Maintain shiptrack index files database
<LI><A HREF="doc/html/dat2gmt.html"> dat2gmt</A> Convert ASCII listing to .gmt file
<LI><A HREF="doc/html/gmt2bin.html"> gmt2bin</A> Create bin-index files from .gmt files
<LI><A HREF="doc/html/gmt2dat.html"> gmt2dat</A> Write .gmt file as ASCII listing
<LI><A HREF="doc/html/gmtinfo.html"> gmtinfo</A> Report statistics of .gmt files
<LI><A HREF="doc/html/gmtlegs.html"> gmtlegs</A> Find all ship tracks in given area
<LI><A HREF="doc/html/gmtlist.html"> gmtlist</A> Extract data from .gmt files
<LI><A HREF="doc/html/gmtpath.html"> gmtpath</A> Find actual path to .gmt files
<LI><A HREF="doc/html/gmttrack.html"> gmttrack</A> Plot ship tracks on maps
<LI><A HREF="doc/html/mgd77togmt.html"> mgd77togmt</A> Convert MGD-77 files to .gmt format
</UL>
<HR>
<H3>The MISC package</H3>
<UL>
<LI><A HREF="doc/html/gmtdigitize.html"> gmtdigitize</A> Digitize map features using a large-format digitizer
<LI><A HREF="doc/html/gmtstitch.html"> gmtstitch</A> Combine line segments that share end-points
<LI><A HREF="doc/html/makepattern.html"> makepattern</A> Make GMT color pattern from b/w pattern
<LI><A HREF="doc/html/ps2raster.html"> ps2raster</A> Convert PostScript to raster image
<LI><A HREF="doc/html/psmegaplot.html"> psmegaplot</A> Make poster-size plot using tiling
<LI><A HREF="doc/html/nc2xy.html"> nc2xy</A> Convert netCDF column file to ASCII xy data
</UL>
<HR>
<H3>The SEGYPROGS package</H3>
<UL>
<LI><A HREF="doc/html/pssegy.html"> pssegy</A> Plot SEGY seismic data files in 2-D
<LI><A HREF="doc/html/pssegyz.html"> pssegyz</A> Plot SEGY seismic data files in 3-D
</UL>
<HR>
<H3>The SPOTTER package</H3>
<UL>
<LI><A HREF="doc/html/backtracker.html"> backtracker</A> Forward and backward flowlines or hotspot tracks
<LI><A HREF="doc/html/grdrotater.html"> grdrotater</A> Rotate entire grids using a finite rotation
<LI><A HREF="doc/html/hotspotter.html"> hotspotter</A> Make CVA grids by convolving flowlines and seamount shapes
<LI><A HREF="doc/html/originator.html"> originator</A> Associate seamounts with hotspot point sources
<LI><A HREF="doc/html/rotconverter.html"> rotconverter</A> Manipulate finite and stage rotations
</UL>
<HR>
<H3>The X2SYS package</H3>
<UL>
<LI><A HREF="doc/html/x2sys_binlist.html"> x2sys_binlist</A> Create bin-index files from generic track files
<LI><A HREF="doc/html/x2sys_cross.html"> x2sys_cross</A> Generic track intersection (Crossover) detector
<LI><A HREF="doc/html/x2sys_datalist.html"> x2sys_datalist</A> Generic data extractor for ASCII and binary track data
<LI><A HREF="doc/html/x2sys_get.html"> x2sys_get</A> Find all track files in a given area
<LI><A HREF="doc/html/x2sys_init.html"> x2sys_init</A> Initialize a new index file database for track data
<LI><A HREF="doc/html/x2sys_put.html"> x2sys_put</A> Maintain index files database for track data
</UL>
<HR>
<H3>The X_SYSTEM package</H3>
<UL>
<LI><A HREF="doc/html/x_system.html"> x_system</A> MGG-specific track intersection (Crossover) System Overview
<LI><A HREF="doc/html/x_edit.html"> x_edit</A> Convert between ascii and binary crossover correction tables
<LI><A HREF="doc/html/x_init.html"> x_init</A> Initialize a new crossover data base
<LI><A HREF="doc/html/x_list.html"> x_list</A> Extract crossover data from data base
<LI><A HREF="doc/html/x_over.html"> x_over</A> Calculate crossovers between two MGG cruise files in *.gmt format
<LI><A HREF="doc/html/x_remove.html"> x_remove</A> Remove crossover information from data base
<LI><A HREF="doc/html/x_report.html"> x_report</A> Summarize crossover statistics from data base
<LI><A HREF="doc/html/x_setup.html"> x_setup</A> Determine which cruises need to be compared to current files
<LI><A HREF="doc/html/x_solve_dc_drift.html"> x_solve_dc_drift</A> Find best-fitting offset and drift-rates
<LI><A HREF="doc/html/x_update.html"> x_update</A> Update data base with new crossover information
</UL>
<HR>
<H3>The XGRID package</H3>
<UL>
<LI>xgridedit  A GUI editor of gridfiles
</UL>
<P>
<A HREF="gmt_services.html">
<IMG SRC="gmt_back.gif" alt="RETURN">
Return to GMT Online Services page.
</A>
</BODY>
</HTML>
EOF

# Clean up

rm -f $$.*
