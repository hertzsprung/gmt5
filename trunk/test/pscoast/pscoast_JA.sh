#!/bin/sh
#
#	$Id: pscoast_JA.sh,v 1.1 2008-02-01 22:23:11 guru Exp $
# Make sure when fixed it works for all resolutions -D?

ps=pscoast_JA.ps

. ../functions.sh
header "Test pscoast for JA plot of Germany"

pscoast -JA13:25/52:31/10/7i -Rg -Gred -Sblue -Di -P > $ps

pscmp
