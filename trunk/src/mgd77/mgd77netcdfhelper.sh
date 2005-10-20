#!/bin/sh
#
#	$Id: mgd77netcdfhelper.sh,v 1.4 2005-10-20 12:38:06 pwessel Exp $
#
#	Author:	P. Wessel
#	Date:	2005-OCT-14
#
# This script will create three functions from info in mgd77.h:
#
# MGD77_Read_Header_Params	: Read the MGD77 header attributes from the netCDF file
# MGD77_Write_Header_Params	: Write the MGD77 header attributes from the netCDF file
#
# Code is placed in the file mgd77_functions.h which is included in mgd77.c
#
# The script is run when needed by the makefile
#

cat << EOF > mgd77_functions.h

/* START OF CODE GENERATED BY mgd77netcdfhelper.sh */

void MGD77_Read_Header_Params (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P)
{
	/* Read the netCDF-encoded MGD77 header parameters as attributes of the data set */
	
EOF
# 1. strip out the structure members only (except Record_Type), then replace brackets and ; with space to simplify processing in the loop below */
sed -n '/START OF MGD77_HEADER_PARAMS/,/END OF MGD77_HEADER_PARAMS/p' mgd77.h | egrep -v 'OF MGD77_HEADER_PARAMS|Sequence No|Record_Type' | tr '[];' '   ' > $$.1
while read type name L M; do		# We need a separate read/write statement for each attribute
	pre=""				# Normally, no prefix for character arrays
	cast=""
	n_item=1
	fmt="%s"
	if [ "X$L" = "X" ]; then	# No number means a single character
		length=1
		pre="&"			# We need to take address of a single char
		fmt="%c"
	elif [ "X$M" = "X" ]; then	# Single text length given
		length="strlen (${pre}P->$name)"
	else				# 2-D text array, dim and length given, calc total size
		n_item=$L
		length=`echo $M $L | awk '{print $1*$2}'`
		cast="(char *)"
	fi
	if [ $n_item -ne 7 ]; then
		echo "	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "\"$name\"", ${cast}${pre}P->$name));" >> mgd77_functions.h
		echo "	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "\"$name\"", $length, ${cast}${pre}P->$name));" >> $$.2
		echo "	printf (\"%s %44s :${fmt}\n\", F->NGDC_id, \"$name\", P->$name);" >> $$.3
	else
		cast=""
		length=`echo $M | awk '{print $1-1}'`
		j=0
		while [ $j -lt $n_item ]; do
			k=$j
			j=`expr $j + 1`
			length="strlen (${pre}P->${name}[$k])"
			echo "	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "\"${name}_$j\"", ${cast}${pre}P->$name[$k]));" >> mgd77_functions.h
			echo "	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "\"${name}_$j\"", $length, ${cast}${pre}P->$name[$k]));" >> $$.2
			echo "	printf (\"%s %44s :${fmt}\n\", F->NGDC_id, \"$name\", P->$name[$k]);" >> $$.3
		done
	fi
done < $$.1
cat << EOF >> mgd77_functions.h
}

void MGD77_Write_Header_Params (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P)
{
	/* Write the MGD77 header parameters as attributes of the netCDF-encoded data set */
	
EOF
cat $$.2 >> mgd77_functions.h
cat << EOF >> mgd77_functions.h
}

void MGD77_Dump_Header_Params (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P)
{
	/* Write all the individual MGD77 header parameters to stdout */

EOF
cat $$.3 >> mgd77_functions.h
cat << EOF >> mgd77_functions.h
}

/* END OF CODE GENERATED BY mgd77netcdfhelper.sh */

EOF

rm -f $$.*
echo "mgd77netcdfhelper.sh: mgd77_functions.h created"
