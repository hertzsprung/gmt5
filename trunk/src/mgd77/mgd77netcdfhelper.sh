#!/bin/bash
#
#   $Id$
#
#   Author:     P. Wessel
#   Date:       2005-OCT-14
#   Revised:    2007-JUN-06: Now store _REVISED attributes set by E77
#
# This script will automatically create three functions from info in mgd77.h:
#
# MGD77_Read_Header_Params  : Read the MGD77 header attributes from the netCDF file
# MGD77_Write_Header_Params : Write the MGD77 header attributes to the netCDF file
# MGD77_Dump_Header_Params  : Display individual header attributes, one per line
#
# Code is placed in the file mgd77_functions.h which is included in mgd77_functions.c
#
# The script should be run manually when necessary
#

cat << EOF > mgd77_functions.h
/* \$Id:\$
 *
 * Code automatically generated by mgd77netcdfhelper.sh
 * To be included by mgd77_functions.c
 *
 * Copyright (c) 2005-2012 by P. Wessel
 * See README file for copying and redistribution conditions.
 */

#ifndef _MGD77_FUNCTIONS_H
#define _MGD77_FUNCTIONS_H

#include "mgd77.h"

struct MGD77_HEADER_LOOKUP {    /* Book-keeping for one header parameter  */
        char name[64];          /* Name of this parameter (e.g., "Gravity_Sampling_Rate") */
        GMT_LONG length;        /* Number of bytes to use */
        int record;             /* Header record number where it occurs (1-24) */
        int item;               /* Sequential item order in this record (1->) */
        GMT_LONG check;         /* TRUE if we actually do a test on this item */
        GMT_LONG revised;       /* TRUE if read in via the _REVISED attribute */
        char *ptr[2];           /* Pointers to the corresponding named variable in struct MGD77_HEADER_PARAMS (orig and revised) */
};

struct MGD77_HEADER_PARAMS {            /* See MGD-77 Documentation from NGDC for details */
  /* Sequence No 01: */
  char Record_Type;
EOF

cat << EOF > mgd77_functions.c
/* \$Id:\$
 *
 * Code automatically generated by mgd77netcdfhelper.sh
 *
 * Copyright (c) 2005-2012 by P. Wessel
 * See README file for copying and redistribution conditions.
 */

#include "mgd77_functions.h"

void MGD77_Read_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P)
{
	/* Read the netCDF-encoded MGD77 header parameters as attributes of the data set.
	 * If orig is TRUE we will recover the original MGD77 parameters; otherwise we first
	 * look for revised parameters and fall back on the original if no revision is found. */

	struct MGD77_HEADER_LOOKUP *L;
	int i;

	L = MGD77_Header_Lookup;

EOF
# 1. strip out the structure members (except Record_Type)
key=0
n_check=0
type="char"
last="01"
egrep -v '^#|Record_Type' mgd77_header.txt > $$.1
while read name rec item size check; do
	if [ ! $last = $rec ]; then
		echo "	/* Sequence No $rec: */" >> mgd77_functions.h
	fi
	# We need a separate read/write statement for each attribute
	pre=""                  # Normally, no prefix for character arrays
	cast=""
	n_item=1
	fmt="%s"
	R=`echo $rec  | awk '{printf "%d\n", $1}'`
	I=`echo $item | awk '{printf "%d\n", $1}'`
	REV="L[MGD77_Param_Key(C,$R,$I)].revised"
	n=`echo $size | awk -F'*' '{print NF}'`
	if [ $n -eq 1 ]; then   # Single item
		if [ $size -eq 1 ]; then
			echo "	char $name;" >> mgd77_functions.h
		else
			echo "	char $name[$size];" >> mgd77_functions.h
		fi
		L=$size
		M=0
	else                    # 2-D array
		L=`echo $size | awk -F'*' '{print $1}'`
		M=`echo $size | awk -F'*' '{print $2}'`
		echo "	char $name[$L][$M];" >> mgd77_functions.h
	fi
	if [ $L -eq 1 ]; then   # Single character
		length1=1
		length2=1
		pre="&"             # We need to take address of a single char
#		fmt="%c"
	elif [ $M -eq 0 ]; then # Single text length given
		length1="strlen (${pre}P[0]->$name)"
		length2="strlen (${pre}P[1]->$name)"
	else                    # 2-D text array, dim and length given, calc total size
		n_item=$L
		length1=`echo $M $L | awk '{print $1*$2}'`
		length2=$length1
		cast="(char *)"
	fi
	if [ $L -eq 1 ]; then   # Special handling since these are single characters that may be \0
		echo "	$REV = MGD77_Get_Param (C, F, "\"$name\"", ${cast}${pre}P[0]->$name, ${cast}${pre}P[1]->$name);" >> mgd77_functions.c
		echo "	MGD77_Put_Param (C, F, "\"$name\"", (size_t)$length1, ${cast}${pre}P[0]->$name, (size_t)$length2, ${cast}${pre}P[1]->$name, $REV);" >> $$.2
		echo "	(void) nc_del_att (F->nc_id, NC_GLOBAL, "\"${name}_REVISED\"");" >> $$.5
		# The next line gives "      Parameter_Name :Value".  This format is deliberate in that we may want to
		# use awk -F: to separate out the parameter ($1) and the value ($2). Remember Value could be a sentence with spaces!
		echo "	word[0] = P->$name;" >> $$.3
		echo "	if (F->Want_Header_Item[$key]) printf (\"%s %44s : ${fmt}%c\", F->NGDC_id, \"$name\", word, EOL);" >> $$.3
		echo "	for (i = 0; i < 2; i++) H[$key].ptr[i] = ${cast}${pre}P[i]->$name;" >> $$.7
	else
		echo "	$REV = MGD77_Get_Param (C, F, "\"$name\"", ${cast}${pre}P[0]->$name, ${cast}${pre}P[1]->$name);" >> mgd77_functions.c
		echo "	MGD77_Put_Param (C, F, "\"$name\"", $length1, ${cast}${pre}P[0]->$name, $length2, ${cast}${pre}P[1]->$name, $REV);" >> $$.2
		echo "	(void) nc_del_att (F->nc_id, NC_GLOBAL, "\"${name}_REVISED\"");" >> $$.5
		# The next line gives "      Parameter_Name :Value".  This format is deliberate in that we may want to
		# use awk -F: to separate out the parameter ($1) and the value ($2). Remember Value could be a sentence with spaces!
		echo "	if (F->Want_Header_Item[$key]) printf (\"%s %44s : ${fmt}%c\", F->NGDC_id, \"$name\", P->$name, EOL);" >> $$.3
		echo "	for (i = 0; i < 2; i++) H[$key].ptr[i] = ${cast}${pre}P[i]->$name;" >> $$.7
	fi
	if [ $check = "Y" ]; then
		echo "	\"$name\" $L $rec $item TRUE FALSE NULL" >> $$.6
	else
		echo "	\"$name\" $L $rec $item FALSE FALSE NULL" >> $$.6
	fi
	key=`expr $key + 1`
	last=$rec
done < $$.1

n_names=`cat $$.6 | wc -l | awk '{printf "%d\n", $1}'`
cat << EOF >> mgd77_functions.c

	for (i = 0, F->revised = FALSE; !F->revised && i < MGD77_N_HEADER_PARAMS; i++) if (L[i].revised) F->revised = TRUE;
}

void MGD77_Write_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P)
{
	/* Write the MGD77 header parameters as attributes of the netCDF-encoded data set */

	struct MGD77_HEADER_LOOKUP *L;

	L = MGD77_Header_Lookup;

EOF
cat $$.2 >> mgd77_functions.c
cat << EOF >> mgd77_functions.c
}

void MGD77_Dump_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P)
{
	char word[2] = { '\0', '\0'}, EOL = '\n';

	/* Write all the individual MGD77 header parameters to stdout */

EOF
cat $$.3 >> mgd77_functions.c
cat << EOF >> mgd77_functions.c
}

void MGD77_Reset_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F)
{
	/* Remove the revised MGD77 header attributes so we return to the original values.
	 * Here we simply ignore return values since many of these are presumably unknown attributes.
	 * File is assumed to be in define mode. */

EOF
cat $$.5 >> mgd77_functions.c
cat << EOF >> mgd77_functions.c
	(void) nc_del_att (F->nc_id, NC_GLOBAL, "E77");
}

GMT_LONG MGD77_Get_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, char *value_orig, char *value_rev)
{	/* Get a single parameter: original if requested, otherwise check for revised value first */
	GMT_LONG got_rev = FALSE;

	if (!F->original) { /* Must look for revised attribute unless explicitly turned off [ e.g, mgd77convert -FC] */
		char Att[64] = {""};
		sprintf (Att, "%s_REVISED", name); /* Revised attributes have _REVISED at the end of their names */
		if (nc_get_att_text (F->nc_id, NC_GLOBAL, Att, value_rev) == NC_NOERR) got_rev = TRUE;	/* Found a revised attribute */
	}

	/* Next, we get the original value */

	MGD77_nc_status (C, nc_get_att_text (F->nc_id, NC_GLOBAL, name, value_orig));
	return (got_rev);
}

void MGD77_Put_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, size_t length_orig, char *value_orig, size_t length_rev, char *value_revised, GMT_LONG revised)
{	/* Function assumes we are in define mode.
	 * Place a single parameter in one of several ways:
	 * revised == 2: Only write the revised attribute [This only happens in mgd77manage where we update a value via -Ae]
	 * revised == 1: Write both revised and original attribute; [e.g., mgd77manage -D needs this]
	 * revised == 0: Only write original attribute;
	 * If F->original is TRUE place a revised attribute name.
	 */

	if (revised == 2 || revised == 0) MGD77_nc_status (C, nc_put_att_text (F->nc_id, NC_GLOBAL, name, length_orig, value_orig));
	if (revised) { /* Write revised attribute */
		char Att[64] = {""};
		sprintf (Att, "%s_REVISED", name); /* Revised attributes have _REVISED at the end of their names */
		MGD77_nc_status (C, nc_put_att_text (F->nc_id, NC_GLOBAL, Att, length_rev, value_revised));
	}
}
EOF
cat << EOF >> mgd77_functions.h
};

void MGD77_Write_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P);

GMT_LONG MGD77_Get_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, char *value_orig, char *value_revised);
void MGD77_Put_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, size_t length_orig, char *value_orig, size_t length_rev, char *value_revised, GMT_LONG revised);
void MGD77_Read_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P);
void MGD77_Dump_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P);
void MGD77_Reset_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F);
void MGD77_Init_Ptr (struct GMT_CTRL *C, struct MGD77_HEADER_LOOKUP *H, struct MGD77_HEADER_PARAMS **P);
int MGD77_Param_Key (struct GMT_CTRL *C, GMT_LONG record, int item);

#define MGD77_N_HEADER_PARAMS $n_names

extern struct MGD77_HEADER_LOOKUP MGD77_Header_Lookup[];

#endif /* _MGD77_FUNCTIONS_H */
EOF

cat << EOF >> mgd77_functions.c

struct MGD77_HEADER_LOOKUP MGD77_Header_Lookup[MGD77_N_HEADER_PARAMS] = {
EOF
awk '{printf "\t{ %-46s, %3d, %2d, %2d, %5s, %5s, { %s, %s } },\n", $1, $2, $3, $4, $5, $6, $7, $7}' $$.6 >> mgd77_functions.c
cat << EOF >> mgd77_functions.c
};

void MGD77_Init_Ptr (struct GMT_CTRL *C, struct MGD77_HEADER_LOOKUP *H, struct MGD77_HEADER_PARAMS **P)
{	/* Assigns array of pointers to each idividual parameter */
	int i;

EOF
cat $$.7 >> mgd77_functions.c
cat << EOF >> mgd77_functions.c
}
EOF

rm -f $$.*
echo "mgd77netcdfhelper.sh: mgd77_functions.[ch] created"
