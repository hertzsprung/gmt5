#!/bin/sh
#
#	$Id: grdvolume.sh,v 1.9 2010-06-21 23:55:22 guru Exp $

makegrd () {
xyz2grd -I1 -Gt.nc -Za $1 <<%
1
1
1
1
1
0
1
0
0
%
echo
echo xyz2grd $1 \; grdvolume $2 -Sk:
grdvolume t.nc $2 -Sk
echo xyz2grd $1 \; grdvolume $2:
grdvolume t.nc $2
echo xyz2grd $1 \; grdvolume $2 -L-1:
grdvolume t.nc $2 -L-1
}

testcase () {
makegrd -R0/2/0/2
makegrd "-R0/3/0/3 -F"
makegrd "-R0/2/0/2 -N0"
makegrd "-R0/3/0/3 -F -N0"
makegrd -R0/2/0/2 "-C0/0.8/0.4"
}

. ../functions.sh
header "Test grdvolume for various grid registrations"

testcase > grdvolume.log

diff grdvolume.log grdvolume.out > log

if [ $? != 0 ]; then
	touch fail
fi

rm -f t.nc .gmtcommands4 log

passfail grdvolume
