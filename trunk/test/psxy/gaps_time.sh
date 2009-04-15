#!/bin/sh
# Test psxy plotting with gaps
. ../functions.sh

ps=gaps_time.ps
header "Test plotting time series with NaNs indicating line gaps"

gmtset NAN_RECORDS pass TIME_UNIT y TIME_EPOCH 2000-01-01 PLOT_DATE_FORMAT mm
gmtmath "gtec_tx_daily.nc?time/gtec" -C0 1 MOD = | \
	psxy -f0t -R2000-01-01T/2001-01-01T/5/65 -gx0.5 -JX6i/3i -W0.5p,blue -P -B1O:Month:/20f5:GTEC: > $ps

pscmp
