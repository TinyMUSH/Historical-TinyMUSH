/* unsplit.c - filter for re-combining continuation lines */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int main(argc, argv)
int argc;
char **argv;
{
	int c, numcr;

	while ((c = getchar()) != EOF) {
		if (c == '\\') {
			numcr = 0;
			do {
				c = getchar();
				if (c == '\n')
					numcr++;
			} while ((c != EOF) && isspace(c));
			if (numcr > 1)
				putchar('\n');
			ungetc(c, stdin);
		} else {
			putchar(c);
		}
	}
	fflush(stdout);
	exit(0);
}
