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

#define N 10000	/* Lazy so just assume max this many suppressions */

int main () {
	char *item[N], *key[N];
	char buffer[10000] = {""}, top[10000] = {""}, line[BUFSIZ] = {""};
	char *s = NULL;
	unsigned int n_unique = 0, n_in = 0, k, j = 0, n_gmt = 0, first;
	
	while (fgets (line, BUFSIZ, stdin)) {	/* Read until EOF */
		if (line[0] == '{') {	/* Start of a suppression block report */
			buffer[0] = 0;	/* Reset buffer for a new entry */
			fgets (line, BUFSIZ, stdin);	/* Skip the first line which is "<insert_a_suppression_name_here>" */
			first = 1;
			while (fgets (line, BUFSIZ, stdin) && line[0] != '}') {	/* While not seeing the ending brace... */
				if (!strncmp (line, "   fun:", 7U) && first) {
					strcpy (top, line);
					first = 0;
				}
				strcat (buffer, line);	/* ...we append the line to the buffer */
			}
			n_in++;
			/* See if we already have this one */
			for (k = j = 0; j == 0 && k < n_unique; k++) {
				if (!strcmp (item[k], buffer)) j = 1;	/* Found a duplicate */
			}
			if (j == 0) {	/* New entry */
				s = strdup (top);
				if (s == NULL) {
					fprintf (stderr, "Ran out of memory - get a bigger computer!\n");
					exit (-1);
				}
				key[n_unique] = s;
				s = strdup (buffer);
				if (s == NULL) {
					fprintf (stderr, "Ran out of memory - get a bigger computer!\n");
					exit (-1);
				}
				item[n_unique++] = s;
				if (n_unique == N) {
					fprintf (stderr, "Recompile after increasing N\n");
					exit (-1);
				}
			}
		}
	}
	/* Now print out the unique entries, giving them unique names in the process */
	for (k = j = 0; k < n_unique; k++) {
		if (strstr (key[k], "GMT_") || strstr (key[k], "PSL_") || strstr (key[k], "gmt_") || strstr (key[k], "psl_"))	{	/* This one ends in a GMT/PSL call so we DONT want to suppress it */
			n_gmt++;
			fprintf (stderr, "{\n%s}\n", item[k]);
			continue;
		}
		printf ("{\n   GMT-suppress-%6.6d\n", j++);	/* Unique name */
		printf ("%s", item[k]);
		printf ("}\n");
	}
	fprintf (stderr, "Found %d suppressions, only %d were unique, and we saved %d after skipping %d that were GMT/PSL-specific.\n", n_in, n_unique, j, n_gmt);
	/* Free memory */
	for (k = 0; k < n_unique; k++) {
		free (item[k]);
		free (key[k]);
	}
}
