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

#include "bdb.h"	/* required by code */
#include "udb.h"	/* required by code */

#ifndef STANDALONE
extern void FDECL(dump_database_internal, (int));
#endif

#define DEFAULT_DBMCHUNKFILE "mudDB"

static char *dbfile = DEFAULT_DBMCHUNKFILE;
static int db_initted = 0;

DB_ENV *dbenvp = NULL;
static DB *dbp = NULL;
static DBT dat;
static DBT key;
static DB_TXN *tid = NULL;

int FDECL(dddb_del, (Aname *));
int FDECL(dddb_put, (Attr *, Aname *));
extern void VDECL(fatal, (char *, ...));
extern void VDECL(logf, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

static void dbm_error(errpfx, msg)
const char *errpfx;
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

int dddb_init()
{
	static char *copen = "db_init cannot open ";
	int block_size;
	int cache_size;
	int ret;
	
	/* Calculate the proper page size */
	
	for (block_size = 1; block_size < LBUF_SIZE; block_size = block_size << 1) ;

	/* Set the cache size to the twice the page size. It only needs to
	 * be large enough to hold the biggest piece of data we can store
	 * plus a few bytes of overhead, since we flush the Sleepycat cache
	 * after each write.
	 */
	 
	cache_size = 2 * block_size;

#ifndef STANDALONE
	/* Open the database environment handle */
	
	if ((ret = db_env_create(&dbenvp, 0)) != 0) {
		log_printf("Could not open database environment handle.\n");
		return(1);
	}

	/* Set the database cache size */
	
	if ((ret = dbenvp->set_cachesize(dbenvp, 0, cache_size, 0)) != 0) {
		dbp->err(dbp, ret, "set_cachesize");
		return(1);
	}

	/* Open the database environment */
	
	if ((ret = dbenvp->open(dbenvp, mudconf.dbhome,
	     DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|DB_RECOVER|DB_USE_ENVIRON|DB_CREATE, 0600)) != 0) {
		log_printf("Could not open database environment.\n");
		return(1);
	}

	/* Open the database cursor */
	
	if ((ret = db_create(&dbp, dbenvp, 0)) != 0) {
#else
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
#endif
		log_printf("Could not open database handle.\n");
		return(1);
	}

	/* Set the error call hook */
	
#ifndef STANDALONE
	dbenvp->set_errcall(dbenvp, dbm_error);	
#endif
	dbp->set_errcall(dbp, dbm_error);
	
	/* Set the database page size */

	if ((ret = dbp->set_pagesize(dbp, block_size)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		return(1);
	}

	/* Open the database */
	
	if ((ret = dbp->open(dbp, dbfile, NULL, DB_HASH, DB_CREATE, 0600)) != 0) {
		dbp->err(dbp, ret, "%s", "DB->open");
		return(1);
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
	int ret;
	
	/* Checkpoint the database to ensure that all transactions
	   are committed */

#ifndef STANDALONE
	if (dbp != NULL) {
		if ((ret = txn_checkpoint(dbenvp, 0, 0, 0)) != 0) {
			dbenvp->err(dbenvp, ret, "%s", dbfile);
			return(1);
		}
	}
#endif
	
	/* Close the database */
	if (dbp != NULL) {
		if ((ret = dbp->close(dbp, 0)) != 0) {
			dbp->err(dbp, ret, "%s", dbfile);
			return(1);
		}
		dbp = NULL;
	}

#ifndef STANDALONE
	/* Close the database environment */
	if (dbenvp != NULL) {
		if ((ret = dbenvp->close(dbenvp, 0)) != 0) {
			dbenvp->err(dbenvp, ret, "%s", dbfile);
			return(1);
		}
		dbenvp = NULL;
	}
#endif
	db_initted = 0;
	return (0);
}

Attr *dddb_get(nam)
Aname *nam;
{
	Attr *atr;
	int ret;
	
	if (!db_initted)
		return (NULL);

	memset(&key, 0, sizeof(key));
	memset(&dat, 0, sizeof(dat));

	key.data = (char *)nam;
	key.size = sizeof(Aname);

#ifndef STANDALONE
	/* Start an atomic, recoverable transaction */
	
	if ((ret = txn_begin(dbenvp, NULL, &tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_begin");
#endif

	/* Fetch the data */

	switch (ret = dbp->get(dbp, tid, &key, &dat, 0)) {
	case DB_LOCK_DEADLOCK:
		/* This should never happen... if we ever thread the server,
		   some logic should be added here. */
		   
		STARTLOG(LOG_ALWAYS, "DB", "DEADLOCK")
			log_printf("Database is deadlocked. Exiting.\n");
		ENDLOG
		exit(0);
	case 0:
		break;
	case DB_NOTFOUND:
#ifndef STANDALONE
		if (txn_commit(tid, 0))
			dbenvp->err(dbenvp, ret, "txn_commit");
#endif
		dbp->err(dbp, ret, "dbp->get");
		log_db_err(nam->object, nam->attrnum, "db_get: not found");
		return(NULL);
	}

#ifndef STANDALONE
	/* The transaction finished, commit it */

	if ((ret = txn_commit(tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_commit");
#endif
	/* if the file is badly formatted, ret == NULL */

	if ((atr = attrfromFILE(dat.data)) == NULL) {
		log_db_err(nam->object, nam->attrnum,
		           "db_get: cannot decode");
		return NULL;
	}
	
	return (atr);
}

int dddb_put(atr, nam)
Attr *atr;
Aname *nam;
{
	int nsiz;
	int ret;

	if (!db_initted)
		return (1);

	nsiz = strlen(atr) + 1;

	memset(&key, 0, sizeof(key));
	memset(&dat, 0, sizeof(dat));

	key.data = (char *)nam;
	key.size = sizeof(Aname);
	
	/* make table entry */
	dat.data = (char *)XMALLOC(nsiz, "dddb_put");
	dat.size = nsiz;

	if (attrtoFILE(atr, dat.data) != 0) {
		log_db_err(nam->object, nam->attrnum, "db_put: can't save");
		XFREE(dat.data, "dddb_put");
		return (1);
	}
	
#ifndef STANDALONE
	/* Begin an atomic, recoverable transaction */

	if ((ret = txn_begin(dbenvp, NULL, &tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_begin");
#endif

	/* Store the value */

	switch (ret = dbp->put(dbp, tid, &key, &dat, 0)) {
	case DB_LOCK_DEADLOCK:
		/* This should never happen... if we ever thread the server,
		   some logic should be added here. */
		   
		STARTLOG(LOG_ALWAYS, "DB", "DEADLOCK")
			log_printf("Database is deadlocked. Exiting.\n");
		ENDLOG
		exit(0);
	case 0:
		break;
	default:
#ifndef STANDALONE
		txn_abort(tid);
#endif
		dbp->err(dbp, ret, "dbp->put");
		XFREE(dat.data, "dddb_put");
		return (1);
	}

#ifndef STANDALONE
	/* The transaction finished, commit it. */
	if ((ret = txn_commit(tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_commit");
#endif

	/* Perform a sync, since we only have enough space to do
	 * one write */

	if ((ret = dbp->sync(dbp, 0)) != 0)
		dbp->err(dbp, ret, "sync");

	XFREE(dat.data, "dddb_put");
	return (0);
}

int dddb_del(nam)
Aname *nam;
{
	int ret;

	if (!db_initted) {
		return (-1);
	}

	memset(&key, 0, sizeof(key));

	key.data = (char *)nam;
	key.size = sizeof(Aname);

#ifndef STANDALONE
	/* Begin an atomic, recoverable transaction */

	if ((ret = txn_begin(dbenvp, NULL, &tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_begin");
#endif

	/* Delete the key */

	switch (ret = dbp->del(dbp, tid, &key, 0)) {
	case DB_LOCK_DEADLOCK:
		/* This should never happen... if we ever thread the server,
		   some logic should be added here. */
		   
		STARTLOG(LOG_ALWAYS, "DB", "DEADLOCK")
			log_printf("Database is deadlocked. Exiting.\n");
		ENDLOG
		exit(0);
	case 0:
		break;
	case DB_NOTFOUND:
#ifndef STANDALONE
		txn_abort(tid);
#endif
		return(0);		
	default:
#ifndef STANDALONE
		txn_abort(tid);
#endif
		dbp->err(dbp, ret, "dbp->del");
		return (1);
	}

#ifndef STANDALONE
	/* The transaction finished, commit it. */
	if ((ret = txn_commit(tid, 0)) != 0)
		dbenvp->err(dbenvp, ret, "txn_commit");
#endif

	/* Perform a sync, since we only have enough space to do
	 * one write */

	if ((ret = dbp->sync(dbp, 0)) != 0)
		dbp->err(dbp, ret, "sync");

	return (0);
}
