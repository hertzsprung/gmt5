#!/bin/sh
#	$Id: make_math.sh,v 1.5 2007-09-24 16:22:43 remko Exp $

# This script puts together Xmath.h, Xmath_def.h, Xmath_explain.h, and Xmath_man.i
# from Xmath.c.  To be run from the GMT src directory.  X is either grd or gmt.
#
# Usage: make_math.sh grd|gmt [-s]
# -s for silent operation

prefix=$1
gush=1
if [ $# = 2 ]; then	# Passed optional second argument (-s) to be silent
	gush=0
fi
PRE=`echo $prefix | awk '{print toupper($1)}'`

if [ $gush = 1 ]; then
	echo "Making ${prefix}math_def.h"
fi
rm -f ${prefix}math_def.h

# First gather operator descriptions from source file

grep "^/\*OPERATOR: " ${prefix}math.c | sort > $$.txt
n_op=(`cat $$.txt | wc -l`)

# Add backward compability

cat << EOF > ${prefix}math_def.h
/*--------------------------------------------------------------------
 *
 *	${prefix}math_def.h [Generated by make_math.sh]
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------
 *	${prefix}math_def.h is automatically generated by make_math.sh;
 *	Do NOT edit manually!
 */

#define ${PRE}MATH_N_OPERATORS	$n_op
#define ${PRE}MATH_STACK_SIZE	$n_op

/* For backward compatibility: */

EOF
awk '{ if ($2 == "ADD") {printf "#define ADD\t%d\n", NR-1} \
	else if ($2 == "DIV") {printf "#define DIV\t%d\n", NR-1} \
	else if ($2 == "MUL") {printf "#define MUL\t%d\n", NR-1} \
	else if ($2 == "POW") {printf "#define POW\t%d\n", NR-1} \
	else if ($2 == "SUB") {printf "#define SUB\t%d\n", NR-1}}' $$.txt >> ${prefix}math_def.h
echo "" >> ${prefix}math_def.h

# Add function declarations

echo "/* Declare all functions to return int */" >> ${prefix}math_def.h
echo "" >> ${prefix}math_def.h
if [ $1 = "gmt" ]; then
	awk '{printf "void table_%s(struct GMTMATH_INFO *info, double **stack[], BOOLEAN *constant, double *factor, int last, int start, int n);\t\t/* id = %d */\n", $2, NR-1}' $$.txt >> gmtmath_def.h
else
	awk '{printf "void grd_%s(struct GRDMATH_INFO *info, float *stack[], BOOLEAN *constant, double *factor, int last);\t\t/* id=%d */\n", $2, NR-1}' $$.txt >> grdmath_def.h
fi
echo "" >> ${prefix}math_def.h

# Define operator array
echo "/* Declare operator array */" >> ${prefix}math_def.h
echo "" >> ${prefix}math_def.h
echo "char *operator[${PRE}MATH_N_OPERATORS] = {" >> ${prefix}math_def.h
awk -v n_op=$n_op '{if (NR < n_op) {printf "\t\"%s\",\t\t/* id = %d */\n", $2, NR-1} else {printf "\t\"%s\"\t\t/* id = %d */\n", $2, NR-1}}' $$.txt >> ${prefix}math_def.h
echo "};" >> ${prefix}math_def.h
echo "" >> ${prefix}math_def.h

# Make usage explanation include file

rm -f ${prefix}math_explain.h
if [ $gush = 1 ]; then
	echo "Making ${prefix}math_explain.h"
fi
cat << EOF > ${prefix}math_explain.h
/*--------------------------------------------------------------------
 *
 *	${prefix}math_explain.h [Generated by make_math.sh]
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*	${prefix}math_explain.h is automatically generated by make_math.sh;
 *	Do NOT edit manually!
 *
 */
EOF
awk '{ \
	printf "\t\tfprintf (stderr, \"\t%s\t%d %d\t%s", $2, $3, $4, $5; \
	for (i = 6; i <= NF-1; i++) printf " %s", $i; \
	printf "\\n\");\n" \
}' $$.txt | sed -e 's/%/%%/g' >> ${prefix}math_explain.h

# Make ${prefix}math_init function

if [ $gush = 1 ]; then
	echo "Making ${prefix}math.h"
fi
cat << EOF > ${prefix}math.h
/*--------------------------------------------------------------------
 *
 *	${prefix}math.h [Generated by make_math.sh]
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*	${prefix}math.h is automatically generated by make_math.sh;
 *	Do NOT edit manually!
 */

void ${prefix}math_init (PFV ops[], int n_args[], int n_out[])
{

	/* Operator function		# of operands  		# of outputs */

EOF
if [ $1 = "gmt" ]; then
	awk '{ printf "\tops[%d] = table_%s;\t\tn_args[%d] = %d;\t\tn_out[%d] = %d;\n", NR-1, $2, NR-1, $3, NR-1, $4}' $$.txt >> ${prefix}math.h
else
	awk '{ printf "\tops[%d] = grd_%s;\t\tn_args[%d] = %d;\t\tn_out[%d] = %d;\n", NR-1, $2, NR-1, $3, NR-1, $4}' $$.txt >> grdmath.h
fi
echo "}" >> ${prefix}math.h

# Make man page explanation include file

if [ $gush = 1 ]; then
	echo "Making ${prefix}math_man.i"
fi
awk '{ \
	a = $2"         ";
	printf "\\fB%s\\fP\t%d %d\t%s", substr(a, 0, 9), $3, $4, $5; \
	a = index($5,sprintf("%c",39)); \
	for (i = 6; i <= NF-1; i++) \
	{ \
		printf " %s", $i; \
		if(index($i,sprintf("%c",39))) ++a; \
	} \
	if(a) printf "\\\"%c",39; \
	printf "\n.br\n" \
}' $$.txt > ${prefix}math_man.i

rm -f $$.txt
