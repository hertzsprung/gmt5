#!/bin/bash
#	$Id$
#
# Convert grids between netcdf and several of the other "native" formats

log=reformat_rf.log

grdmath -R-10/10/-10/10 -I1 X = lixo.nc

# First conver to int
grdreformat lixo.nc lixo.rf=rf
grdmath lixo.nc lixo.rf=rf SUB = lixo_dif.nc
grd2xyz lixo_dif.nc -ZTLa > $log

# Now convert back to .nc
grdreformat lixo.rf=rf lixo.nc
grdmath lixo.nc lixo.rf=rf SUB = lixo_dif.nc
grd2xyz lixo_dif.nc -ZTLa >> $log

res=`minmax -C $log`
echo ${res[0]} ${res[1]} | $AWK '{if($1 != 0 || $2 != 0) print 1}' > fail
