/*
 * A tool to traverse a corrupted GDBM database, look for special tags, and
 * rebuild a consistent database
 */

#include "autoconf.h"
#include "config.h"
#include "db.h"
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>

#include "gdbmdefs.h"

static gdbm_file_info *dbp = NULL;

static void gdbm_panic(mesg)
{
	fprintf(stderr, "GDBM panic: %s\n", mesg);
}

int main(ac, av)
int ac;
char *av[];
{
	datum key, dat;
	char newfile[256];
	FILE *f;
	int numbytes, filesize;
	long filepos;
	struct stat buf;
	bucket_element be;
	int c;
		
	if (ac != 2) {
		fprintf(stderr, "usage: %s <database name>\n", av[0]);
		exit(0);
	}
	
	/* Open files */
	
	sprintf(newfile, "%s.new", av[1]);
	
	if ((dbp = gdbm_open(newfile, 8192, GDBM_WRCREAT, 0600, gdbm_panic)) == NULL) {
		fprintf(stderr, "Can't open gdbm database %s\n", newfile);
		exit(0);
	}
	
	if (stat(av[1], &buf)) {
		fprintf(stderr, "could not stat %s\n", av[1]);
		exit(0);
	}

	filesize = buf.st_size;
	
	f = fopen(av[1], "r");
	
	while (fread((void *)&c, 1, 1, f) != 0) {
		/* Quick and dirty */
		if ((char)c == 'T') {
			filepos = ftell(f);
			
			/* Rewind one byte */
			fseek(f, -1, SEEK_CUR);
			
			if (fread((void *)&be, sizeof(bucket_element),
				  1, f) == 0) {
				fprintf(stderr, "Could not read	at %d\n",
					filepos);
				exit(0);
			}
			
			if (!memcmp((void *)(be.start_tag), (void *)"TM3S", 4) &&
			    be.data_pointer < filesize &&
			    be.key_size < filesize &&
			    be.data_size < filesize) {
				filepos = ftell(f);
				
				/* Seek to where the data begins */
				fseek(f, be.data_pointer, SEEK_SET);
				
				key.dptr = (char *)malloc(be.key_size);
				key.dsize = be.key_size;
				dat.dptr = (char *)malloc(be.data_size);
				dat.dsize = be.data_size;
				
				if ((numbytes = fread((void *)(key.dptr), 1,
					  key.dsize, f)) == 0) {
					fprintf(stderr, "Could not read	at %d\n",
						filepos);
					exit(0);
				}
				
				if (fread((void *)(dat.dptr), dat.dsize,
					  1, f) == 0) {
					fprintf(stderr, "Could not read	at %d\n",
						filepos);
					exit(0);
				}
				
				fprintf(stdout, "%d:%d:%d:%s\n", numbytes, ((Aname *)key.dptr)->object, ((Aname *)key.dptr)->attrnum, (char *)dat.dptr);
				
				if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
					fprintf(stderr, "Cannot write to GDBM database\n");
					exit(0);
				}
				
				free(key.dptr);
				free(dat.dptr);
				
				/* Seek back to where we left off */
				
				fseek(f, filepos, SEEK_SET);
			} else {
				/* Seek back to one byte after we started
				 * and continue */
				
				fseek(f, filepos, SEEK_SET);
			}
		}		
	}
	
	fclose(f);
	gdbm_close(dbp);
	exit(1);
}
