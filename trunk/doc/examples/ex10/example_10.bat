REM		GMT EXAMPLE 10
REM
REM		$Id$
REM
REM Purpose:	Make 3-D bar graph on top of perspective map
REM GMT progs:	pscoast, pstext, psxyz
REM DOS calls:	echo, del, gawk
REM
echo GMT EXAMPLE 10
set ps=example_10.ps
gmt pscoast -Rd -JX8id/5id -Dc -Slightblue -Glightbrown -Wfaint -A1000 -p200/40 -K > %ps%
echo {print $1, $2, $3} > awk.txt
gawk -f awk.txt agu2008.d | gmt pstext -R -J -O -K -p -D-0.2i/0 -F+f20p,Helvetica-Bold,blue=thinner+jRM >> %ps%
gmt psxyz agu2008.d -R-180/180/-90/90/1.01/100000 -J -JZ2.5il -So0.3ib1 -Gdarkgreen -Wthinner -Bx60g60 -By30g30 -Bza1p+lMemberships -BWSneZ+t"AGU 2008 Membership Distribution" -O -p >> %ps%
del awk.txt .gmt*
