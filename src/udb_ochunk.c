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
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "gdbm.h"	/* required by code */
#include "udb.h"	/* required by code */

#define DEFAULT_DBMCHUNKFILE "mudDB"

static char *dbfile = DEFAULT_DBMCHUNKFILE;
static int db_initted = 0;

static GDBM_FILE dbp = (GDBM_FILE) 0;

static datum dat;
static datum key;

extern void VDECL(fatal, (char *, ...));
extern void VDECL(logf, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

void dddb_setsync(flag)
int flag;
{
	char *gdbm_error;
	
	if (gdbm_setopt(dbp, GDBM_SYNCMODE, &flag, sizeof(int)) == -1) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf("setsync: cannot toggle sync flag", dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
	}
}

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
	int i;

	i = gdbm_reorganize(dbp);
	return i;
}

int dddb_init()
{
	static char *copen = "db_init cannot open ";
	char tmpfile[256];
	char *gdbm_error;
	int block_size;
	int i;
	
	/* Calculate the proper page size */
	
	for (block_size = 1; block_size < LBUF_SIZE; block_size = block_size << 1) ;

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
	
	/* Set the cache size to be two hash buckets for GDBM. */
	 
	i = 2;
	if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}

	/* Set GDBM to manage a global free space table. */
	 
	i = 1;
	if (gdbm_setopt(dbp, GDBM_CENTFREE, &i, sizeof(int)) == -1) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}

	/* Set GDBM to coalesce free blocks. */
	 
	i = 1;
	if (gdbm_setopt(dbp, GDBM_COALESCEBLKS, &i, sizeof(int)) == -1) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}

#ifdef STANDALONE
	/* If we're standalone, having GDBM wait for each write is a
	 * performance no-no; run non-synchronous */
	
	dddb_setsync(0);
#endif

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

/* Pass dddb_get a key and its length, and it returns data through
 * dataptr and length through datalenptr. Type is used as part of the GDBM
 * key to guard against namespace conflicts in different MUSH subsystems */

void dddb_get(keydata, keylen, dataptr, datalenptr, type)
void *keydata;
int keylen;
void **dataptr;
int *datalenptr;
int type;
{
	char *s;
#ifdef TEST_MALLOC
	char *newdat;
#endif
	
	if (!db_initted) {
		if (dataptr)
			*dataptr = NULL;
		return;
	}
	
	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)XMALLOC(sizeof(int) + keylen, "dddb_get");
	memcpy((void *)s, keydata, keylen); 
	s += keylen;
	memcpy((void *)s, (void *)&type, sizeof(int));
	key.dsize = sizeof(int) + keylen;

	dat = gdbm_fetch(dbp, key);

#ifdef TEST_MALLOC
	/* We must XMALLOC() our own memory */
	if (dat.dptr != NULL) {
		newdat = (char *)XMALLOC(dat.dsize, "dddb_get.newdat");
		memcpy(newdat, dat.dptr, dat.dsize);
		free(dat.dptr);
		dat.dptr = newdat;
	}
#endif
	
	if (dataptr)
		*dataptr = dat.dptr;
	if (datalenptr)
		*datalenptr = dat.dsize;

	XFREE(key.dptr, "dddb_get");
}

/* Pass dddb_put a key and its length, data and its length, and the type
 * of entry you are storing */

int dddb_put(keydata, keylen, data, len, type)
void *keydata;
int keylen;
void *data;
int len;
{
	char *s;
	
	if (!db_initted)
		return (1);

	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)XMALLOC(sizeof(int) + keylen, "dddb_put");
	memcpy((void *)s, keydata, keylen); 
	s += keylen;
	memcpy((void *)s, (void *)&type, sizeof(int));
	key.dsize = sizeof(int) + keylen;
	
	/* make table entry */
	dat.dptr = (char *)XMALLOC(len, "dddb_put.dat");
	memcpy(dat.dptr, data, len);
	dat.dsize = len;

	if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
		logf("db_put: can't gdbm_store ", " ", (char *)-1, "\n", (char *)0);
		XFREE(dat.dptr, "dddb_put.dat");
		XFREE(key.dptr, "dddb_put.key");
		return (1);
	}

	XFREE(dat.dptr, "dddb_put.dat");
	XFREE(key.dptr, "dddb_put.key");

	return (0);
}

/* Pass dddb_del a key and its length, and the type of entry you are
 * deleting */

int dddb_del(keydata, keylen, type)
void *keydata;
int keylen;
int type;
{
	char *s;
	
	if (!db_initted) {
		return (-1);
	}

	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)XMALLOC(sizeof(int) + keylen, "dddb_del");
	memcpy((void *)s, keydata, keylen); 
	s += keylen;
	memcpy((void *)s, (void *)&type, sizeof(int));
	key.dsize = sizeof(int) + keylen;

	dat = gdbm_fetch(dbp, key); 

	/* not there? */
	if (dat.dptr == NULL) {
		XFREE(key.dptr, "dddb_del.key");
		return (0);
	}

#ifdef TEST_MALLOC
	free(dat.dptr);
#else
	XFREE(dat.dptr, "dddb_del.dat");
#endif

	/* drop key from db */
	if (gdbm_delete(dbp, key)) {
		logf("db_del: can't delete key\n", (char *)NULL);
		XFREE(key.dptr, "dddb_del.key");
		return (1);
	}
	XFREE(key.dptr, "dddb_del.key");
	return (0);
}
