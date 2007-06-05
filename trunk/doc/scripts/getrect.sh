#!/bin/sh
#       $Id: getrect.sh,v 1.2 2007-06-05 15:44:51 remko Exp $
#
# Expects xmin xmax ymin ymax in km relative to map center
# -R and -J are set by preceding GMT commands
(echo -$1 -$1; echo -$1 $1; echo $1 $1; echo $1 -$1) | mapproject -R -J -I -Fk -C
