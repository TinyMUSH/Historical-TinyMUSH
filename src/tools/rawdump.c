
/*
 * A tool to open and traverse the dbm database that goes with
 * your game, and dump out a raw report of where every object lives in
 * the chunkfile, and how many bytes it takes up.
 */
#include "autoconf.h"
#include "config.h"

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>

#include "gdbmdefs.h"
/* #include "gdbm.h" */

static gdbm_file_info *dbp = NULL;

static void gdbm_panic(mesg)
{
	fprintf(stderr, "GDBM panic: %s\n", mesg);
}

int main(ac, av)
int ac;
char *av[];
{
	int obj;
	datum key, dat;
	int new_hash_val; /* temp stuff for findkey operation */
	char *temp;
	int elem_loc, ofs, siz;

	if (ac != 2) {
		fprintf(stderr, "usage: %s <database name>\n", av[0]);
		exit(0);
	}
	/*
	 * open hash table 
	 */
	if ((dbp = gdbm_open(av[1], 8192, GDBM_WRCREAT, 0600, gdbm_panic)) == NULL) {
		fprintf(stderr, "Can't open gdbm database %s\n", av[1]);
		exit(0);
	}
	key = gdbm_firstkey(dbp);
	while (key.dptr != (char *)NULL) {
		elem_loc = _gdbm_findkey(dbp, key, &temp, &new_hash_val);

		if (elem_loc == -1) {
			fprintf(stderr, "gdbm database %s inconsistent\n", av[1]);
			exit(0);
		}

		ofs = dbp->bucket->h_table[elem_loc].data_pointer;
		siz = (dbp->bucket->h_table[elem_loc].key_size +
		       dbp->bucket->h_table[elem_loc].data_size);

		bcopy(key.dptr, (char *)&obj, sizeof(obj));

		printf("Object %d resides at offset %d and takes %d bytes\n",
		       obj, ofs, siz);

		key = gdbm_nextkey(dbp, key);
	}

	exit(1);
}
