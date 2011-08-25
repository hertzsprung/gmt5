#!/bin/sh
#	$Id$
#
# Make the GMT ftpsite hitmap PNG image and the web page that uses it.
# The files created are gmt_hitmap.png and gmt_mirrors.html
#
dpi=100
yoff=1
sdia=0.2
mdia=0.15
#-------------------------------------------------------------------------------------------------
# ADD NEW MIRRORS HERE (Also fix install script etc)
# Remember: Only ONE TAB between fields, otherwise the next awk gets confused.
cat << EOF > mirrors.d
-77:0	38:52	ftp://ibis.grdl.noaa.gov/pub/gmt5	CT	SILVER SPRING - USA	NOAA, Lab for Satellite Altimetry, Silver Spring, Maryland, USA	Serving East North America
-122:22	47:40	ftp://ftp.iris.washington.edu/pub/gmt5	CB	SEATTLE - USA	IRIS (Incorporated Research Institutions for Seismology), Seattle, Washington, USA	Serving West North America
-46:40	-23:32	ftp://ftp.iag.usp.br/pub/gmt5	CT	S\303O PAULO - BRAZIL	IAG-USP, Dept of Geophysics, S&#227;o Paulo, Brazil	Serving South America
10:44	59:55	ftp://ftp.geologi.uio.no/pub/gmt5	CB	OSLO - NORWAY	IFG, Dept of Geosciences, Oslo, Norway	Serving Europe
16:22	48:12	ftp://gd.tuwien.ac.at/pub/gmt5	CT	VIENNA - AUSTRIA	Vienna U of Techology, Vienna, Austria	Serving Europe
138:30	35:00	ftp://ftp.scc.u-tokai.ac.jp/pub/gmt5	CB	SHIMIZU - JAPAN	Tokai U, Shimizu, Japan	Serving Asia
151:12.6	-33:51	ftp://mirror.geosci.usyd.edu.au/pub/gmt5	LT	SYDNEY - AUSTRALIA	School of Geosciences, U of Sydney	Serving Australia
23:00	-30:00	ftp://gmt.mirror.ac.za/pub/gmt5	LT	CAPE TOWN - S AFRICA	TENET, Tertiary Education & Research Networks of South Africa, South Africa	Serving Africa
EOF
cat << EOF > master.d
-157:59	21:55	ftp://ftp.soest.hawaii.edu/gmt5	CT	HONOLULU - USA	SOEST, Dept of Geology & Geophysics, Honolulu, Hawaii, USA	Serving the Pacific Rim
EOF
#-------------------------------------------------------------------------------------------------
if [ $# -eq 1 ]; then
	gush=0
else
	gush=1
fi
if [ $gush ]; then
	echo "gmt_hitmap.sh: Preparing the web page hitmap image"
fi
awk -F'\t' '{printf "%s\t%s\t%s\t%s\n", $1, $2, $4, $5}' mirrors.d > mirror_sites.d
awk -F'\t' '{printf "%s\t%s\t%s\t%s\n", $1, $2, $4, $5}' master.d  > master_site.d

clon=193
pscoast -Rd -JKs$clon/6i -Sazure1 -Gburlywood -Dc -A2000 -Bg60/g30 -K -P -Y${yoff}i --PS_CHAR_ENCODING=ISOLatin1+ > gmt_hitmap.ps
# Draw spokes from Hawaii to each site
i=1
while read lon lat rest; do
	cut -f1,2 master.d > t
	echo "$lon $lat" >> t
	psxy -R -J -O -K -W2p t -A >> gmt_hitmap.ps
done < mirror_sites.d
# Place seagreen and red circles
psxy -R -J -O -K -Sc${mdia}i -Gseagreen -Wthin mirror_sites.d -N >> gmt_hitmap.ps
psxy -R -J -O -K -Sc${sdia}i -Gred -Wthin master_site.d >> gmt_hitmap.ps
psxy -R -J -O -K -Sc0.1i -Gwhite -Wthin master_site.d >> gmt_hitmap.ps
# Add site labels
pstext -R -J -O -K mirror_sites.d -Gwhite -W0.25p -TO -N -Dj0i/0.175i -F+f9,Helvetica-Bold+j >> gmt_hitmap.ps
pstext -R -J -O -K master_site.d -Gnavy -W0.25p -TO -N -Dj0.15i/0.2i -F+f12p,Helvetica-Bold,white+j >> gmt_hitmap.ps
# Draw the legend
pslegend -R0/5/0/1 -Jx1i -O -K -D3/0/2.1i/0.6i/CT -Gcornsilk -Y-0.1i -F2p -L1.25 --FONT_ANNOT_PRIMARY=14p,Helvetica-Bold << EOF >> gmt_hitmap.ps
S 0.2i c 0.2i red 0.25p 0.45i GMT Master Site
S 0.2i c 0.15i seagreen 0.25p 0.45i GMT Mirror Site
EOF
pslegend -R0/5/0/1 -Jx1i -O -K -D3/0/2.1i/0.6i/CT -L1.25 --FONT_ANNOT_PRIMARY=14p,Helvetica-Bold << EOF >> gmt_hitmap.ps
S 0.2i c 0.1i white 0.25p 0.45i 
EOF
psxy -R -J -O /dev/null >> gmt_hitmap.ps
ps2raster -A -E$dpi -Tg gmt_hitmap.ps
if [ $gush ]; then
	open gmt_hitmap.png
fi
file gmt_hitmap.png > $$
IW=`awk '{print $5}' $$`
IH=`awk '{print $6}' $$`
if [ $gush ]; then
	echo "gmt_hitmap.sh: Preparing gmt_mirrors.html"
fi
H=`echo 180 90 | mapproject -JN180/6i -Rd | cut -f2`
width=`gmtmath -Q 6 $dpi MUL =`
height=`gmtmath -Q $H $dpi MUL =`
mrad=`gmtmath -Q $mdia $dpi MUL 2 DIV =`
srad=`gmtmath -Q $sdia $dpi MUL 2 DIV =`
cat << EOF > gmt_mirrors.html
<HTML>
<!--    gmt_mirrors.html [Generated by gmt_hitmap.sh]      -->
<HEAD>
<TITLE>GMT - The Generic Mapping Tools</TITLE>
</HEAD>
<BODY bgcolor="#ffffff">
<CENTER><H2>GMT Master and Mirror FTP Sites</H2></CENTER>
You only need to visit one of these sites if you cannot (or will not) do the automatic
install described under the <A HREF="gmt_download.html">Download</A> section. This is most likely true
if you just want to obtain Windows executables. To connect to your preferred ftp server, click on the
location that is physically nearest you:<P>
<P><CENTER><IMG SRC="gmt_hitmap.png" WIDTH="$IW" HEIGHT="$IH" ALIGN=bottom NATURALSIZEFLAG="3" USEMAP="#GMTmap"> </A></CENTER>
<MAP name="GMTmap">
EOF
awk '{print $1, $2}' master.d  | mapproject -JN180/$width -Rd | awk -F'\t' '{printf "\t<AREA HREF=\"%s\" SHAPE=\"circle\" COORDS=\"%d,%d,%d\">\n", $3, int($1+0.5), int('$height'-$2+0.5), int('$srad'+0.5)}' >> gmt_mirrors.html
awk '{print $1, $2}' mirrors.d | mapproject -JN180/$width -Rd | awk -F'\t' '{printf "\t<AREA HREF=\"%s\" SHAPE=\"circle\" COORDS=\"%d,%d,%d\">\n", $3, int($1+0.5), int('$height'-$2+0.5), int('$mrad'+0.5)}' >> gmt_mirrors.html
cat << EOF >> gmt_mirrors.html
</MAP>
<P>
Alternatively, click on the corresponding text link below:
<OL>
EOF
cat master.d mirrors.d | awk -F'\t' '{printf "<LI><A HREF=\"%s\">%s</A>.  %s\n", $3, $6, $7}' >> gmt_mirrors.html
cat << EOF >> gmt_mirrors.html
</OL>
</BODY>
</HTML>
EOF
rm -f mirrors.d master.d mirror_sites.d master_site.d gmt_hitmap.ps $$
