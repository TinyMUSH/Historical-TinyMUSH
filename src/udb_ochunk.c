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
	STARTLOG(LOG_ALWAYS, "DB", "ERROR")
		log_printf("Database error: %s\n", msg);
	ENDLOG
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
	int i;
	
	if (!mudstate.standalone)
		sprintf(tmpfile, "%s/%s", mudconf.dbhome, dbfile);
	else
		strcpy(tmpfile, dbfile);
 
	if ((dbp = gdbm_open(tmpfile, mudstate.db_block_size, GDBM_WRCREAT|GDBM_SYNC|GDBM_NOLOCK, 0600, dbm_error)) == (GDBM_FILE) 0) {
		gdbm_error = (char *)gdbm_strerror(gdbm_errno);
		logf(copen, tmpfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
		return (1);
	}
	

	if (mudstate.standalone) {
		/* Set the cache size to be 400 hash buckets for GDBM. */

		i = 400;
		if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1) {
			gdbm_error = (char *)gdbm_strerror(gdbm_errno);
			logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
			return (1);
		}
	} else {
		/* Set the cache size to be two hash buckets for GDBM. */
	 
		i = 2;
		if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1) {
			gdbm_error = (char *)gdbm_strerror(gdbm_errno);
			logf(copen, dbfile, " ", (char *)-1, "\n", gdbm_error, "\n", (char *)0);
			return (1);
		}
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

	/* If we're standalone, having GDBM wait for each write is a
	 * performance no-no; run non-synchronous */
	
	if (mudstate.standalone)
		dddb_setsync(0);

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

/* Pass db_get a key, and it returns data. Type is used as part of the GDBM
 * key to guard against namespace conflicts in different MUSH subsystems */

DBData db_get(gamekey, type)
DBData gamekey;
unsigned int type;
{
	DBData gamedata;
	char *s;
#ifdef TEST_MALLOC
	char *newdat;
#endif
	
	if (!db_initted) {
		gamedata.dptr = NULL;
		gamedata.dsize = 0;
		return gamedata;
	}
	
	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)RAW_MALLOC(sizeof(int) + gamekey.dsize, "db_get");
	memcpy((void *)s, gamekey.dptr, gamekey.dsize); 
	s += gamekey.dsize;
	memcpy((void *)s, (void *)&type, sizeof(unsigned int));
	key.dsize = sizeof(int) + gamekey.dsize;

	dat = gdbm_fetch(dbp, key);

#ifdef TEST_MALLOC
	/* We must XMALLOC() our own memory */
	if (dat.dptr != NULL) {
		newdat = (char *)XMALLOC(dat.dsize, "db_get.newdat");
		memcpy(newdat, dat.dptr, dat.dsize);
		free(dat.dptr);
		dat.dptr = newdat;
	}
#endif
	
	gamedata.dptr = dat.dptr;
	gamedata.dsize = dat.dsize;

	RAW_FREE(key.dptr, "db_get");
	return gamedata;
}

/* Pass db_put a key, data and the type of entry you are storing */

int db_put(gamekey, gamedata, type)
DBData gamekey;
DBData gamedata;
unsigned int type;
{
	char *s;
	
	if (!db_initted)
		return (1);

	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)RAW_MALLOC(sizeof(int) + gamekey.dsize, "db_put");
	memcpy((void *)s, gamekey.dptr, gamekey.dsize); 
	s += gamekey.dsize;
	memcpy((void *)s, (void *)&type, sizeof(unsigned int));
	key.dsize = sizeof(int) + gamekey.dsize;
	
	/* make table entry */
	dat.dptr = gamedata.dptr;
	dat.dsize = gamedata.dsize;

	if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
		logf("db_put: can't gdbm_store ", " ", (char *)-1, "\n", (char *)0);
		RAW_FREE(dat.dptr, "db_put.dat");
		RAW_FREE(key.dptr, "db_put");
		return (1);
	}

	RAW_FREE(key.dptr, "db_put");

	return (0);
}

/* Pass db_del a key and the type of entry you are deleting */

int db_del(gamekey, type)
DBData gamekey;
unsigned int type;
{
	char *s;
	
	if (!db_initted) {
		return (-1);
	}

	/* Construct a key (GDBM likes first 4 bytes to be unique) */
	
	s = key.dptr = (char *)RAW_MALLOC(sizeof(int) + gamekey.dsize, "db_del");
	memcpy((void *)s, gamekey.dptr, gamekey.dsize); 
	s += gamekey.dsize;
	memcpy((void *)s, (void *)&type, sizeof(unsigned int));
	key.dsize = sizeof(int) + gamekey.dsize;

	dat = gdbm_fetch(dbp, key); 

	/* not there? */
	if (dat.dptr == NULL) {
		RAW_FREE(key.dptr, "db_del.key");
		return (0);
	}

#ifdef TEST_MALLOC
	free(dat.dptr);
#else
	RAW_FREE(dat.dptr, "db_del.dat");
#endif

	/* drop key from db */
	if (gdbm_delete(dbp, key)) {
		logf("db_del: can't delete key\n", (char *)NULL);
		RAW_FREE(key.dptr, "db_del.key");
		return (1);
	}
	RAW_FREE(key.dptr, "db_del.key");
	return (0);
}
