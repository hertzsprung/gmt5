#!/bin/sh
#-----------------------------------------------------------------------------
#	 $Id: webman.sh,v 1.2 2001-03-21 17:41:03 pwessel Exp $
#
#	webman.csh - Automatic generation of the GMT web manual pages
#
#	Author:	Paul Wessel
#	Date:	17-SEP-1999
#	Version: 1.1 Bourne shell
#
#	Uses the man2html program + alot of sed and awk...
#	Assumes a cvs update has occured so files are fresh.
#
#	Must be run from the main GMT directory after man pages have
#	been made.
#
#	On HP (raptor) we must do an extra sed substitution since some
#	blank lines comes through as lines with ?9 at the end or line
#	(? is some unknown character).  This sed line is added to the
#	webman1.sed file
#-----------------------------------------------------------------------------

if [ $#argv = 1 ]; then	# If -s is given we run silently with defaults
	gush=0
else			# else we make alot of noise
	gush=1
fi

mkdir -p www/gmt/doc/html

# First make a list of all the GMT programs

sed -e 's/\./ /gp' -e 'sB/B Bgp' guru/GMT_all_files.lis | egrep -v 'gmt_|fourt' | awk '{if ($3 == "c") print $2}' | sort -u > GMT_programs.lis

# add GMT to the program list file as well

echo GMT >> GMT_programs.lis

# Now make sed script (actually 3, since sed seems to have trouble with long sed scripts...)

awk '{printf "s%%>%s<%%><A HREF=%c%s.html%c>%s</A><%%g\n", $1, 34, $1, 34, $1}' GMT_programs.lis > webman1.sed
awk '{printf "s%%%s,%%<A HREF=%c%s.html%c>%s</A>,%%g\n", $1, 34, $1, 34, $1}' GMT_programs.lis > webman2.sed
awk '{printf "s%%%s$%%<A HREF=%c%s.html%c>%s</A>%%g\n", $1, 34, $1, 34, $1}' GMT_programs.lis > webman3.sed

# Fix for man on HP that gives 9s in output
echo 's/^.9$//g' >> webman1.sed

# Ok, go to source directory and make html files

for prog in `cat GMT_programs.lis`; do
	if [ $gush = 1 ]; then
		echo "Making ${prog}.html"
	fi
	grep -v ${prog} webman1.sed > this1.sed
	grep -v ${prog} webman2.sed > this2.sed
	grep -v ${prog} webman3.sed > this3.sed
	nroff -man man/manl/${prog}.l | $MAN2HTML -title $prog | sed -f this1.sed | sed -f this2.sed | sed -f this3.sed > www/gmt/doc/html/${prog}.html
	echo '<body bgcolor="#ffffff">' >> www/gmt/doc/html/${prog}.html
done

# Ok, then do the supplemental packages

cd src
for package in cps dbase imgsrc meca mgg misc segyprogs spotter x2sys x_system; do
	cd $package
	for f in *.man; do
		prog=`echo $f | awk -F. '{print $1}'`
		if [ $gush = 1 ]; then
			echo "Making ${prog}.html"
		fi
		nroff -man $f | $MAN2HTML -title $prog | sed -f ../../webman1.sed | sed -f ../../webman2.sed | sed -f ../../webman3.sed > ${prog}.html
		echo '<body bgcolor="#ffffff">' >> ${prog}.html
	done
	cd ..
done
cd ..

#-----------------------------------------------------------------
#	Generate the main GMT Manual page directory
#-----------------------------------------------------------------

cat << EOF > www/gmt/gmt_man.html
<HTML>
<!-- gmt_man.html - Automatically generated by webman.sh  -->
<title>GMT Online Man Pages</title>
<body bgcolor="#ffffff">
<P><center><img src="images/gmt_bar.gif" ALT="---------------------------------"></center></P>
<center><h1>GMT Online Man Pages</h1></center><p>
These man pages are html-versions of the Unix man pages for GMT.  They
are grouped by function. For an alphabetical order, click <A HREF="#anchor_alpha">here</A>.
<h2><A NAME="anchor_theme">Thematic listing of GMT Unix man pages</h2>
<HR>
<h3>GMT WRAPPER:</h3>
<UL>
<LI><A HREF="doc/html/GMT.html"> GMT</A> A Wrapper for GMT programs</LI>
</UL>
<h3>FILTERING OF 1-D AND 2-D DATA:</h3>
<UL>
<LI><A HREF="doc/html/blockmean.html"> blockmean</A> L2 (x,y,z) data filter/decimator</LI>
<LI><A HREF="doc/html/blockmedian.html"> blockmedian</A> L1 (x,y,z) data filter/decimator</LI>
<LI><A HREF="doc/html/blockmode.html"> blockmode</A> Mode-estimating (x,y,z) data filter/decimator</LI>
<LI><A HREF="doc/html/filter1d.html"> filter1d</A> Filter 1-D data (time series)</LI>
<LI><A HREF="doc/html/grdfilter.html"> grdfilter</A> Filter 2-D data in space domain</LI>
</UL>
<h3>PLOTTING OF 1-D and 2-D DATA:</h3>
<UL>
<LI><A HREF="doc/html/grdcontour.html"> grdcontour</A> Contouring of 2-D gridded data</LI>
<LI><A HREF="doc/html/grdimage.html"> grdimage</A> Produce images from 2-D gridded datar</LI>
<LI><A HREF="doc/html/grdvector.html"> grdvector</A> Plot vector fields from 2-D gridded data</LI>
<LI><A HREF="doc/html/grdview.html"> grdview</A> 3-D perspective imaging of 2-D gridded data</LI>
<LI><A HREF="doc/html/psbasemap.html"> psbasemap</A> Create a basemap frame</LI>
<LI><A HREF="doc/html/psclip.html"> psclip</A> Use polygon files as clipping paths</LI>
<LI><A HREF="doc/html/pscoast.html"> pscoast</A> Plot coastlines, filled continents, rivers, and political borders</LI>
<LI><A HREF="doc/html/pscontour.html"> pscontour</A> Direct contouring or imaging of xyz-data by triangulation</LI>
<LI><A HREF="doc/html/pshistogram.html"> pshistogram</A> Plot a histogram</LI>
<LI><A HREF="doc/html/psimage.html"> psimage</A> Plot Sun rasterfiles on a map</LI>
<LI><A HREF="doc/html/psmask.html"> psmask</A> Create overlay to mask specified regions of a map</LI>
<LI><A HREF="doc/html/psrose.html"> psrose</A> Plot sector or rose diagrams</LI>
<LI><A HREF="doc/html/psscale.html"> psscale</A> Plot grayscale or colorscale</LI>
<LI><A HREF="doc/html/pstext.html"> pstext</A> Plot textstrings</LI>
<LI><A HREF="doc/html/pswiggle.html"> pswiggle</A> Draw anomalies along track</LI>
<LI><A HREF="doc/html/psxy.html"> psxy</A> Plot symbols, polygons, and lines in 2-D</LI>
<LI><A HREF="doc/html/psxyz.html"> psxyz</A> Plot symbols, polygons, and lines in 3-D</LI>
</UL>
<h3>GRIDDING OF (X,Y,Z) DATA:</h3>
<UL>
<LI><A HREF="doc/html/nearneighbor.html"> nearneighbor</A> Nearest-neighbor gridding scheme</LI>
<LI><A HREF="doc/html/surface.html"> surface</A> Continuous curvature gridding algorithm</LI>
<LI><A HREF="doc/html/triangulate.html"> triangulate</A> Perform optimal Delauney triangulation on xyz data</LI>
</UL>
<h3>SAMPLING OF 1-D AND 2-D DATA:</h3>
<UL>
<LI><A HREF="doc/html/grdsample.html"> grdsample</A> Resample a 2-D gridded data onto new grid</LI>
<LI><A HREF="doc/html/grdtrack.html"> grdtrack</A> Sampling of 2-D data along 1-D track</LI>
<LI><A HREF="doc/html/sample1d.html"> sample1d</A> Resampling of 1-D data</LI>
</UL>
<h3>PROJECTION AND MAP-TRANSFORMATION:</h3>
<UL>
<LI><A HREF="doc/html/grdproject.html"> grdproject</A> Project gridded data onto new coordinate system</LI>
<LI><A HREF="doc/html/mapproject.html"> mapproject</A> Transformation of coordinate systems</LI>
<LI><A HREF="doc/html/project.html"> project</A> Project data onto lines/great circles</LI>
</UL>
<h3>INFORMATION:</h3>
<UL>
<LI><A HREF="doc/html/gmtdefaults.html"> gmtdefaults</A> List the current default settings</LI>
<LI><A HREF="doc/html/gmtset.html"> gmtset</A> Edit parameters in the .gmtdefaults file</LI>
<LI><A HREF="doc/html/grdinfo.html"> grdinfo</A> Get information about grd files</LI>
<LI><A HREF="doc/html/minmax.html"> minmax</A> Report extreme values in table datafiles</LI>
</UL>
<h3>CONVERT OR EXTRACT SUBSETS OF DATA:</h3>
<UL>
<LI><A HREF="doc/html/gmtconvert.html"> gmtconvert</A> Convert table data from one format to another</LI>
<LI><A HREF="doc/html/gmtmath.html"> gmtmath</A> Reverse Polish calculator for table data</LI>
<LI><A HREF="doc/html/gmtselect.html"> gmtselect</A> Select table subsets based on multiple spatial criteria</LI>
<LI><A HREF="doc/html/grd2xyz.html"> grd2xyz</A> Convert 2-D gridded data to table</LI>
<LI><A HREF="doc/html/grdcut.html"> grdcut</A> Cut a sub-region from a grd file</LI>
<LI><A HREF="doc/html/grdpaste.html"> grdpaste</A> Paste together grdfiles along common edge</LI>
<LI><A HREF="doc/html/grdreformat.html"> grdreformat</A> Convert from one grdformat to another</LI>
<LI><A HREF="doc/html/splitxyz.html"> splitxyz</A> Split xyz files into several segments</LI>
<LI><A HREF="doc/html/xyz2grd.html"> xyz2grd</A> Convert table to 2-D grd file</LI>
</UL>
<h3>MISCELLANEOUS:</h3>
<UL>
<LI><A HREF="doc/html/makecpt.html"> makecpt</A> Create GMT color palette tables</LI>
<LI><A HREF="doc/html/spectrum1d.html"> spectrum1d</A> Compute spectral estimates from time-series</LI>
<LI><A HREF="doc/html/triangulate.html"> triangulate</A> Perform optimal Delauney triangulation on xyz data</LI>
</UL>
<h3>DETERMINE TRENDS IN 1-D AND 2-D DATA:</h3>
<UL>
<LI><A HREF="doc/html/fitcircle.html"> fitcircle</A> Finds best-fitting great or small circles</LI>
<LI><A HREF="doc/html/grdtrend.html"> grdtrend</A> Fits polynomial trends to grdfiles (z = f(x,y))</LI>
<LI><A HREF="doc/html/trend1d.html"> trend1d</A> Fits polynomial or Fourier trends to y = f(x) series</LI>
<LI><A HREF="doc/html/trend2d.html"> trend2d</A> Fits polynomial trends to z = f(x,y) series</LI>
</UL>
<h3>OTHER OPERATIONS ON 2-D GRIDS:</h3>
<UL>
<LI><A HREF="doc/html/grd2cpt.html"> grd2cpt</A> Make color palette table from grdfile</LI>
<LI><A HREF="doc/html/grdclip.html"> grdclip</A> Limit the z-range in gridded data sets</LI>
<LI><A HREF="doc/html/grdedit.html"> grdedit</A> Modify grd header information</LI>
<LI><A HREF="doc/html/grdfft.html"> grdfft</A> Operate on grdfiles in frequency domain</LI>
<LI><A HREF="doc/html/grdgradient.html"> grdgradient</A> Compute directional gradient from grdfiles</LI>
<LI><A HREF="doc/html/grdhisteq.html"> grdhisteq</A> Histogram equalization for grdfiles</LI>
<LI><A HREF="doc/html/grdlandmask.html"> grdlandmask</A> Creates mask grdfile from coastline database</LI>
<LI><A HREF="doc/html/grdmask.html"> grdmask</A> Set nodes outside a clip path to a constant</LI>
<LI><A HREF="doc/html/grdmath.html"> grdmath</A> Reverse Polish calculator for grdfiles</LI>
<LI><A HREF="doc/html/grdvolume.html"> grdvolume</A> Calculating volume under a surface within a contour</LI>
</UL>
<h3>POSTSCRIPT LIBRARY FUNCTION CALLS (for developers):</h3>
<UL>
<LI><A HREF="doc/html/pslib.html"> pslib</A> Reference manual for libpsl.a</LI>
</UL>
<HR>
<h2><A NAME="anchor_alpha">Alphabetical listing of GMT Unix man pages</h2>
<OL>
EOF
grep -v pslib GMT_programs.lis > $$
\rm -f GMT_programs.lis
\mv -f $$ GMT_programs.lis
awk '{printf "<LI><A HREF=%cdoc/html/%s.html%c> %s</A></LI>\n", 34, $1, 34, $1}' GMT_programs.lis >> www/gmt/gmt_man.html
cat << EOF >> www/gmt/gmt_man.html
</OL>
<HR>
For a thematic listing of GMT man pages, click <A HREF="#anchor_theme">here</A>.
<P><center><img src="images/gmt_bar.gif" ALT="---------------------------------"></center></P>
<A HREF="gmt_services.html">
<IMG SRC="images/gmt_back.gif" ALT="RETURN">
Return to GMT Online Services page.
</A>
</HTML>
EOF

#-----------------------------------------------------------------
#	Generate the supplemental GMT Manual page directory
#-----------------------------------------------------------------

cat << EOF > www/gmt/gmt_suppl.html
<HTML>
<!-- gmt_suppl.html - Automatically generated by webman.sh  -->
<title>GMT Supplemental Online Man Pages</title>
<body bgcolor="#ffffff">
<P><center><img src="images/gmt_bar.gif" ALT="---------------------------------"></center></P>
<center><h1>GMT Supplemental Online Man Pages</h1></center><p>
These man pages are html-versions of the Unix man pages for the GMT
supplemental programs, grouped by package.  Note that only the
packages actually installed on your system will be accessible.
<HR>
<h3>The CPS package</h3>
<UL>
<LI><A HREF="doc/html/cpsencode.html"> cpsencode</A> Encoding of Complete PostScript files</LI>
<LI><A HREF="doc/html/cpsdecode.html"> cpsdecode</A> Decoding of Complete PostScript files</LI>
</UL>
<HR>
<h3>The DBASE package</h3>
<UL>
<LI><A HREF="doc/html/grdraster.html"> grdraster</A> Extraction of gridfiles from databases</LI>
</UL>
<HR>
<h3>The IMGSRC package</h3>
<UL>
<LI><A HREF="doc/html/img2grd.html"> img2grd</A> Front-end Bourne script to img2mercgrd</LI>
<LI><A HREF="doc/html/img2mercgrd.html"> img2mercgrd</A> Extracting data from Sandwell/Smith altimetry solutions</LI>
</UL>
<HR>
<h3>The MECA package</h3>
<UL>
<LI><A HREF="doc/html/pscoupe.html"> pscoupe</A> Plot seismic moment tensor and/or double couples on a cross-section</LI>
<LI><A HREF="doc/html/psmeca.html"> psmeca</A> Plot seismic moment tensor and/or double couples on maps</LI>
<LI><A HREF="doc/html/pspolar.html"> pspolar</A> Plot polarities on the lower half-sphere</LI>
<LI><A HREF="doc/html/psvelo.html"> psvelo</A> Plot velocity ellipses, strain crosses, or strain wedges on maps</LI>
</UL>
<HR>
<h3>The MGG package</h3>
<UL>
<LI><A HREF="doc/html/binlegs.html"> binlegs</A> Maintain shiptrack index files database</LI>
<LI><A HREF="doc/html/dat2gmt.html"> dat2gmt</A> Convert ASCII listing to .gmt file</LI>
<LI><A HREF="doc/html/gmt2bin.html"> gmt2bin</A> Create bin-index files from .gmt files</LI>
<LI><A HREF="doc/html/gmt2dat.html"> gmt2dat</A> Write .gmt file as ASCII listing</LI>
<LI><A HREF="doc/html/gmtinfo.html"> gmtinfo</A> Report statistics of .gmt files</LI>
<LI><A HREF="doc/html/gmtlegs.html"> gmtlegs</A> Find all ship tracks in given area</LI>
<LI><A HREF="doc/html/gmtlist.html"> gmtlist</A> Extract data from .gmt files</LI>
<LI><A HREF="doc/html/gmtpath.html"> gmtpath</A> Find actual path to .gmt files</LI>
<LI><A HREF="doc/html/gmttrack.html"> gmttrack</A> Plot ship tracks on maps</LI>
<LI><A HREF="doc/html/mgd77togmt.html"> mgd77togmt</A> Convert MGD-77 files to .gmt format</LI>
</UL>
<HR>
<h3>The MISC package</h3>
<UL>
<!-- <LI><A HREF="doc/html/gmtdigitize.html"> gmtdigitize</A> Digitize features using a digitizer</LI> -->
<LI><A HREF="doc/html/makepattern.html"> makepattern</A> Make GMT color pattern from b/w pattern</LI>
<LI><A HREF="doc/html/psmegaplot.html"> psmegaplot</A> Make poster-size plot using tiling</LI>
</UL>
<HR>
<h3>The SEGYPROGS package</h3>
<UL>
<LI><A HREF="doc/html/pssegy.html"> pssegy</A> Plot SEGY seismic data files in 2-D</LI>
<LI><A HREF="doc/html/pssegyz.html"> pssegyz</A> Plot SEGY seismic data files in 3-D</LI>
</UL>
<HR>
<h3>The SPOTTER package</h3>
<UL>
<LI><A HREF="doc/html/backtracker.html"> backtracker</A> Forward and backward flowlines or hotspot tracks</LI>
<LI><A HREF="doc/html/hotspotter.html"> hotspotter</A> Make CVA grids by convolving flowlines and seamount shapes</LI>
<LI><A HREF="doc/html/originator.html"> originator</A> Associate seamounts with hotspot point sources</LI>
</UL>
<HR>
<h3>The X2SYS package</h3>
<UL>
<LI><A HREF="doc/html/x2sys_cross.html"> x2sys_cross</A> Generic track intersection (Crossover) detector</LI>
<LI><A HREF="doc/html/x2sys_datalist.html"> x2sys_datalist</A> Generic data extractor for ASCII and binary track data</LI>
</UL>
<HR>
<h3>The X_SYSTEM package</h3>
<UL>
<LI><A HREF="doc/html/x_system.html"> x_system</A> MGG-specific track intersection (Crossover) System Overview</LI>
<LI><A HREF="doc/html/x_edit.html"> x_edit</A> Convert between ascii and binary crossover correction tables</LI>
<LI><A HREF="doc/html/x_init.html"> x_init</A> Initialize a new crossover data base</LI>
<LI><A HREF="doc/html/x_list.html"> x_list</A> Extract crossover data from data base</LI>
<LI><A HREF="doc/html/x_over.html"> x_over</A> Calculate crossovers between two MGG cruise files in *.gmt format</LI>
<LI><A HREF="doc/html/x_remove.html"> x_remove</A> Remove crossover information from data base</LI>
<LI><A HREF="doc/html/x_report.html"> x_report</A> Summarize crossover statistics from data base</LI>
<LI><A HREF="doc/html/x_setup.html"> x_setup</A> Determine which cruises need to be compared to current files</LI>
<LI><A HREF="doc/html/x_solve_dc_drift.html"> x_solve_dc_drift</A> Find best-fitting offset and drift-rates</LI>
<LI><A HREF="doc/html/x_update.html"> x_update</A> Update data base with new crossover information</LI>
</UL>
<HR>
<h3>The XGRID package</h3>
<UL>
<LI>xgridedit  A GUI editor of gridfiles</LI>
</UL>
<p>
<A HREF="gmt_services.html">
<IMG SRC="images/gmt_back.gif" ALT="RETURN">
Return to GMT Online Services page.
</A>
</HTML>
EOF

# Clean up

rm -f webman?.sed this?.sed GMT_programs.lis lis
