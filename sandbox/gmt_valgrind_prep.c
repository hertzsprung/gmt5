/* $Id$
 * gmt_valgrind_prep deals with preparing suppression listings for lots of system
 * leaks that make it hard to find just the GMT leaks.  This was built for OSX only.
 * but the principle is generic and it is possible there might be other cases as well,
 * such as other libraries like GDAL etc that is outside our control
 * Follow these steps to build the gmt.supp file.  Once it is built then you would
 * normally just start at step 5.
 *
 * 0. find . -name 'valgrind_*.log' -exec rm -f {} \;
 * 1. Set export VALGRIND_ARGS="--track-origins=yes --leak-check=full --gen-suppressions=all"
 * 2. Run "make test"
 * 3. find . -name 'valgrind_*.log' > t.lis
 * 4. cat `cat t.lis` | gmt_valgrind_prep > gmt.supp
 *    Now you are ready to run calgrind without all those silly messages.  Note you may have
 *    to provide full path or place gmt.supp in the current dir first.
 * 5. export VALGRIND_ARGS="--track-origins=yes --leak-check=full --suppressions=gmt.supp"
 * 6. make test
 *
 * Paul Wessel, June 22, 2015.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 100000	/* Lazy so just assume max this many suppressions */

int main () {
	char buffer[10000] = {""}, line[BUFSIZ] = {""};
	char *item[N];
	char *skip = NULL;
	unsigned int n_items = 0, k, j, n_gmt = 0;
	
	while (fgets (line, BUFSIZ, stdin)) {	/* Read until EOF */
		if (line[0] == '{') {	/* Start of a suppression block report */
			buffer[0] = 0;	/* Reset buffer for a new entry */
			fgets (line, BUFSIZ, stdin);	/* Skip the first line which is "<insert_a_suppression_name_here>" */
			while (fgets (line, BUFSIZ, stdin) && line[0] != '}') {	/* While not seeing the ending brace... */
				strcat (buffer, line);	/* ..we append the line to the buffer */
			}
			if (strstr (buffer, "GMT") || strstr (buffer, "PSL"))	/* This one is for GMT/PSL so we DONT want to include it */
				n_gmt++;
			else	/* Save a copy of this entry */
				item[n_items++] = strdup (buffer);
		}
	}
	/* Done reading, now find and flag duplicates */
	skip = calloc (n_items, sizeof (char));
	for (k = 0; k < n_items; k++) {
		for (j = k + 1; j > n_items; j++) {
			if (skip[j]) continue;	/* Already flagged */
			if (!strcmp (item[k], item[j])) skip[j] = 1;	/* Found a duplicate */
		}
	}
	/* Now print out the unique entries, giving them unique names in the process */
	for (k = j = 0; k < n_items; k++) {
		if (skip[k]) continue;	/* Skip this one */
		printf ("{\n   GMT-suppress-%6.6d\n", j++);	/* Unique name */
		printf ("%s", item[k]);
		printf ("}\n");
	}
	fprintf (stderr, "Found %d suppressions, only %d were unique, and we skipped %d that were GMT/PSL-specific.\n", n_items, j, n_gmt);
	/* Free memory */
	for (k = 0; k < n_items; k++) free (item[k]);
	free (skip);
}
