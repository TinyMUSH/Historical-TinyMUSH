/* udb_ochunk.c */
/* $Id$ */

/*
 * Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mail.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "gdbm.h"	/* required by code */
#include "udb.h"	/* required by code */

#ifndef STANDALONE
extern void FDECL(dump_database_internal, (int));
#endif

#define DEFAULT_DBMCHUNKFILE "mudDB"

static char *dbfile = DEFAULT_DBMCHUNKFILE;
static int db_initted = 0;

static GDBM_FILE dbp = (GDBM_FILE) 0;

static datum dat;
static datum key;

int FDECL(dddb_del, (void *, int));
int FDECL(dddb_put, (void *, int, void *, int));
void *FDECL(dddb_get, (void *, int));

extern void VDECL(fatal, (char *, ...));
extern void VDECL(logf, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

static void dbm_error(msg)
char *msg;
{
#ifndef STANDALONE
	STARTLOG(LOG_ALWAYS, "DB", "ERROR")
		log_printf("Database error: %s\n", msg);
	ENDLOG
#else
	fprintf(mainlog_fp, "Database error: %s\n", msg);
#endif
}

/* gdbm_reorganize compresses unused space in the db */

int dddb_optimize()
{
	return (gdbm_reorganize(dbp));
}

int dddb_init()
{
	static char *copen = "db_init cannot open ";
	char tmpfile[256];
	char *gdbm_error;
	int block_size;
	int cache_size;
	int ret;
	
	/* Calculate the proper page size */
	
	for (block_size = 1; block_size < LBUF_SIZE; block_size = block_size << 1) ;

	/* Set the cache size to be two hash buckets for GDBM. */
	 
	cache_size = 2;
	
#ifndef STANDALONE
	sprintf(tmpfile, "%s/%s", mudconf.dbhome, dbfile);
#else
	strcpy(tmpfile, dbfile);
#endif
 
	if ((dbp = gdbm_open(tmpfile, block_size, GDBM_WRCREAT|GDBM_SYNC|GDBM_NOLOCK, 0600, dbm_error)) == (GDBM_FILE) 0) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}

	if (gdbm_setopt(dbp, GDBM_CACHESIZE, &cache_size, sizeof(int)) == -1) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}

	db_initted = 1;
	return (0);
}

int dddb_setfile(fil)
char *fil;
{
	char *xp;

	if (db_initted)
		return (1);

	/* KNOWN memory leak. can't help it. it's small */
	xp = XSTRDUP(fil, "dddb_setfile");
	if (xp == (char *)0)
		return (1);
	dbfile = xp;
	return (0);
}

int dddb_close()
{
	if (dbp != (GDBM_FILE) 0) {
		gdbm_close(dbp);   
		dbp = (GDBM_FILE) 0;
	}
	db_initted = 0;
	return (0);
}

void *dddb_get(keydata, keylen)
void *keydata;
int keylen;
{
	Attr *atr;
	
	if (!db_initted)
		return (NULL);

	key.dptr = (char *)keydata;
	key.dsize = keylen;
	dat = gdbm_fetch(dbp, key);

	return ((void *)dat.dptr);
}

int dddb_put(keydata, keylen, data, len)
void *keydata;
int keylen;
void *data;
int len;
{
	/* Remember if you put a string here that len is strlen() + 1 */
	
	if (!db_initted)
		return (1);

	key.dptr = (char *)keydata;
	key.dsize = keylen;
	
	/* make table entry */
	dat.dptr = (char *)malloc(len);
	memcpy(dat.dptr, data, len);
	dat.dsize = len;

	if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
		logf("db_put: can't gdbm_store ", " ", (char *)-1, "\n", (char *)0);
		free(dat.dptr);
		return (1);
	}

	free(dat.dptr);

	return (0);
}

int dddb_del(keydata, keylen)
void *keydata;
int keylen;
{
	if (!db_initted) {
		return (-1);
	}

	key.dptr = (char *)keydata;
	key.dsize = keylen;
	dat = gdbm_fetch(dbp, key); 

	/* not there? */
	if (dat.dptr == NULL)
		return (0);

	free(dat.dptr);
	
	/* drop key from db */
	if (gdbm_delete(dbp, key)) {
		logf("db_del: can't delete key\n", (char *)0);
		return (1);
	}
	return (0);
}
