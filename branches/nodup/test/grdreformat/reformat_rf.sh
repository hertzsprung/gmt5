#!/bin/bash
#	$Id$
#
# Convert grids between netcdf and several of the other "native" formats

log=reformat_rf.log

gmt grdmath -R-10/10/-10/10 -I1 X = lixo.nc

# First conver to int
gmt grdreformat lixo.nc lixo.rf=rf
gmt grdmath lixo.nc lixo.rf=rf SUB = lixo_dif.nc
gmt grd2xyz lixo_dif.nc -ZTLa > $log

# Now convert back to .nc
gmt grdreformat lixo.rf=rf lixo.nc
gmt grdmath lixo.nc lixo.rf=rf SUB = lixo_dif.nc
gmt grd2xyz lixo_dif.nc -ZTLa >> $log

res=`gmt minmax -C $log`
echo ${res[0]} ${res[1]} | $AWK '{if($1 != 0 || $2 != 0) print 1}' > fail
