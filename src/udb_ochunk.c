/* udb_ochunk.c - gdbm implementation of untermud chunkfile interface */
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

#include "udb_defs.h"	/* required by code */
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

int FDECL(dddb_del, (Objname *));
int FDECL(dddb_put, (Obj *, Objname *));
extern int FDECL(obj_siz, (Obj *));
extern void VDECL(fatal, (char *, ...));
extern void VDECL(logf, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

/* gdbm_reorganize compresses unused space in the db */

int dddb_optimize()
{
	return (gdbm_reorganize(dbp));
}

static void gdbm_panic(mesg)
char *mesg;
{
	log_printf("GDBM panic: %s\n", mesg);
}

int dddb_init()
{
	static char *copen = "db_init cannot open ";
	char *gdbm_error;
	int block_size;
	int cache_size = 1;
	
	for (block_size = 1; block_size < LBUF_SIZE; block_size = block_size << 1) ;

	if ((dbp = gdbm_open(dbfile, block_size, GDBM_WRCREAT|GDBM_SYNC|GDBM_NOLOCK, 0600, gdbm_panic)) == (GDBM_FILE) 0) {
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

Obj *dddb_get(nam)
Objname *nam;
{
	Obj *ret;
	
	if (!db_initted)
		return ((Obj *) 0);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Objname);
	dat = gdbm_fetch(dbp, key);

	if (dat.dptr == (char *)0) {
		return ((Obj *) 0);
	}
	
	/* if the file is badly formatted, ret == Obj * 0 */
	if ((ret = objfromFILE(dat.dptr)) == (Obj *) 0) {
		logf("db_get: cannot decode ", nam, "\n", (char *)0);
		XFREE(dat.dptr, "dddb_get");
		return NULL;
	}
	
#ifndef STANDALONE	
	/* Check to make sure the requested name and stored name are the
	   same */
	
	if (*nam != ret->name) {
		STARTLOG(LOG_ALWAYS, "BUG", "CRUPT")
			log_printf("Database is corrupt, object %d. Exiting.",
				   (int)ret->name);
		ENDLOG
		raw_broadcast(0, "GAME: Database corruption detected, exiting.");
		exit(8);
	}
#endif
	XFREE(dat.dptr, "dddb_get");
	return (ret);
}

int dddb_put(obj, nam)
Obj *obj;
Objname *nam;
{
	int nsiz;

	if (!db_initted)
		return (1);

	nsiz = obj_siz(obj);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Objname);
	
	/* make table entry */
	dat.dptr = (char *)XMALLOC(nsiz, "dddb_put");
	dat.dsize = nsiz;

	if (objtoFILE(obj, dat.dptr) != 0) {
		logf("db_put: can't save ", nam, " ", (char *)-1, "\n", (char *)0);
		XFREE(dat.dptr, "dddb_put");
		return (1);
	}

	if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
		logf("db_put: can't gdbm_store ", nam, " ", (char *)-1, "\n", (char *)0);
		XFREE(dat.dptr, "dddb_put");
		return (1);
	}

	XFREE(dat.dptr, "dddb_put");
	return (0);
}

int dddb_del(nam)
Objname *nam;
{

	if (!db_initted)
		return (-1);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Objname);
	dat = gdbm_fetch(dbp, key);

	/* not there? */
	if (dat.dptr == (char *)0)
		return (0);

	XFREE(dat.dptr, "dddb_del");
	
	/* drop key from db */
	if (gdbm_delete(dbp, key)) {
		logf("db_del: can't delete key ", nam, "\n", (char *)0);
		return (1);
	}
	return (0);
}
