#!/bin/bash
#	$Id: run_doc_tests.sh,v 1.10 2011-02-28 01:29:03 remko Exp $
#
#	Test newly created plots for documentation against archive
#
# Specify archived images to check against on command line, or otherwise checks all.

echo "Test GMT Documentation EPS files against archive"
echo "--------------------------------------"
echo "File                            STATUS"
echo "--------------------------------------"

# Get the file names of all archived images

if [ $# -eq 0 ] ; then
	origs=orig/*.ps
else
	origs=$*
fi

# Now do the comparison and tally the fails in fail_count.d

rm -f fail_count.d
touch fail_count.d

for o in $origs ; do
        f=`basename $o .ps`
	printf "%-32s" $f.ps
	rms=`compare -density 100 -metric RMSE $f.ps orig/$f.ps $f.png 2>&1`
	if test $? -ne 0; then
        	echo "[FAIL]"
		echo $f: $rms >> fail_count.d
	elif test `echo 40 \> $rms|cut -d' ' -f-3|bc` -eq 1; then
        	echo "[PASS]"
        	rm -f $f.png
	else
        	echo "[FAIL]"
		echo $f: RMS Error = $rms >> fail_count.d
	fi
done

echo "--------------------------------------"
wc -l fail_count.d | awk '{printf "GMT Documentation EPS file failures: %d\n", $1}'
cat fail_count.d
rm -f fail_count.d
echo "--------------------------------------"
