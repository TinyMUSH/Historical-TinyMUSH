/*
 * Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
 * 
 * $Id$
 */

/*
 * configure all options BEFORE including system stuff. 
 */
#include	"autoconf.h"
#include	"udb_defs.h"
#include	"db.h"
#include	"mudconf.h"

#ifdef VMS
#include	<malloc.h>
#include        <types.h>
#include        <file.h>
#include        <unixio.h>
#include        "vms_dbm.h"
#else
#ifndef NEXT
#include	<malloc.h>
#endif /*
        * NEXT 
        */
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/file.h>

#include	"gdbm.h"
#endif /*
        * VMS 
        */

#include	"udb.h"
#include	"config.h"
#include	"externs.h"
#include	"alloc.h"

#ifndef STANDALONE
extern void FDECL(dump_database_internal, (int));
#endif

/*
 * #define              DBMCHUNK_DEBUG
 */

/*
 * default block size to use for bitmapped chunks 
 */
#define	DDDB_BLOCK	256

/*
 * bitmap growth increment in BLOCKS not bytes (512 is 64 BYTES) 
 */
#define	DDDB_BITBLOCK	512

#define	LOGICAL_BLOCK(off)	(off/bsiz)
#define	BLOCK_OFFSET(block)	(block*bsiz)
#define	BLOCKS_NEEDED(siz)	(siz % bsiz ? (siz / bsiz) + 1 : (siz / bsiz))
#define BLOCKS_TO_ALLOC(siz)	BLOCKS_NEEDED(siz) + (BLOCKS_NEEDED(siz) >> 3)

/*
 * dbm-based object storage routines. Somewhat trickier than the default
 * directory hash. a free space list is maintained as a bitmap of free
 * blocks, and an object-pointer list is maintained in a dbm database.
 */
struct hrec {
	off_t off;		/*
				 * Where it lives in the chunk file 
				 */
	int siz;		/*
				 * How long it really is, in bytes 
				 */
	unsigned int blox;	/*
				 * How many blocks it owns 
				 */
};

static int dddb_mark();

#define DEFAULT_DBMCHUNKFILE "mudDB"

static char *dbfile = DEFAULT_DBMCHUNKFILE;
static int bsiz = DDDB_BLOCK;
static int db_initted = 0;
static int last_free = 0;	/*

				 * last known or suspected free block 
				 */

static GDBM_FILE dbp = (GDBM_FILE) 0;

static FILE *dbf = (FILE *) 0;
static struct hrec hbuf;

static char *bitm = (char *)0;
static int bm_top = 0;
static datum dat;
static datum key;

static void growbit();

/*
 * gdbm_reorganize compresses unused space in the db 
 */

int dddb_optimize()
{
	return (gdbm_reorganize(dbp));
}

static void gdbm_panic(mesg)
char *mesg;
{
	log_text(tprintf("GDBM panic: %s\n", mesg));
}

int dddb_init()
{
	static char *copen = "db_init cannot open ";
	char fnam[MAXPATHLEN];
	struct stat sbuf;
	int fxp, block_size;

	/*
	 * now open chunk file 
	 */
	sprintf(fnam, "%s.db", dbfile);

	/*
	 * Bah.  Posix-y systems break "a+", so use this gore instead 
	 */
	if (((dbf = fopen(fnam, "r+")) == (FILE *) 0)
	    && ((dbf = fopen(fnam, "w+")) == (FILE *) 0)) {
		logf(copen, fnam, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
	/*
	 * open hash table 
	 */
	for (block_size = 1; block_size < LBUF_SIZE; block_size = block_size << 1) ;

	if ((dbp = gdbm_open(dbfile, block_size, GDBM_WRCREAT, 0600, gdbm_panic)) == (GDBM_FILE) 0) {
		logf(copen, dbfile, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
	/*
	 * determine size of chunk file for allocation bitmap 
	 */
	sprintf(fnam, "%s.db", dbfile);
	if (stat(fnam, &sbuf)) {
		logf("db_init cannot stat ", fnam, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
	/*
	 * allocate bitmap 
	 */
	growbit(LOGICAL_BLOCK(sbuf.st_size) + DDDB_BITBLOCK);

	key = gdbm_firstkey(dbp);
	while (key.dptr != (char *)0) {
		dat = gdbm_fetch(dbp, key);

		if (dat.dptr == (char *)0) {
			logf("db_init index inconsistent\n", (char *)0);
			return (1);
		}
		bcopy(dat.dptr, (char *)&hbuf, sizeof(hbuf));	/*
								 * alignment 
								 */


		/*
		 * mark it as busy in the bitmap 
		 */
		dddb_mark(LOGICAL_BLOCK(hbuf.off), hbuf.blox, 1);

		key = gdbm_nextkey(dbp, key);
	}

	db_initted = 1;
	return (0);
}

dddb_initted()
{
	return (db_initted);
}

dddb_setbsiz(nbsiz)
int nbsiz;
{
	return (bsiz = nbsiz);
}

dddb_setfile(fil)
char *fil;
{
	char *xp;

	if (db_initted)
		return (1);

	/*
	 * KNOWN memory leak. can't help it. it's small 
	 */
	xp = (char *)malloc((unsigned)strlen(fil) + 1);
	if (xp == (char *)0)
		return (1);
	(void)StringCopy(xp, fil);
	dbfile = xp;
	return (0);
}

int dddb_close()
{
	if (dbf != (FILE *) 0) {
		fclose(dbf);
		dbf = (FILE *) 0;
	}
	if (dbp != (GDBM_FILE) 0) {
		gdbm_close(dbp);
		dbp = (GDBM_FILE) 0;
	}
	if (bitm != (char *)0) {
		free((mall_t) bitm);
		bitm = (char *)0;
		bm_top = 0;
	}
	db_initted = 0;
	return (0);
}


/*
 * grow the bitmap to given size 
 */
static void growbit(maxblok)
int maxblok;
{
	int newtop, newbytes, topbytes;
	char *nbit;

	/*
	 * round up to eight and then some 
	 */
	newtop = (maxblok + 15) & 0xfffffff8;

	if (newtop <= bm_top)
		return;

	newbytes = newtop / 8;
	topbytes = bm_top / 8;
	nbit = (char *)malloc(newbytes);
	if (bitm != (char *)0) {
		bcopy(bitm, (char *)nbit, topbytes);
		free((mall_t) bitm);
	}
	bitm = nbit;

	if (bitm == (char *)0)
		fatal("db_init cannot grow bitmap ", (char *)-1, "\n", (char *)0);

	bzero(bitm + topbytes, (newbytes - topbytes));
	bm_top = newtop;
}



static int dddb_mark(lbn, siz, taken)
off_t lbn;
int siz;
int taken;
{
	int bcnt;

	bcnt = siz;

	/*
	 * remember a free block was here 
	 */
	if (!taken)
		last_free = lbn;

	while (bcnt--) {
		if (lbn >= bm_top - 32)
			growbit(lbn + DDDB_BITBLOCK);

		if (taken)
			bitm[lbn >> 3] |= (1 << (lbn & 7));
		else
			bitm[lbn >> 3] &= ~(1 << (lbn & 7));
		lbn++;
	}
}

static int dddb_alloc(siz)
int siz;
{
	int bcnt;		/*

				 * # of blocks to operate on 
				 */
	int lbn;		/*

				 * logical block offset 
				 */
	int tbcnt;
	int slbn;
	int overthetop = 0;
	int offend = 0;

	lbn = last_free;
	bcnt = BLOCKS_TO_ALLOC(siz);

	while (1) {
		if (lbn >= bm_top - 32) {
			/*
			 * only check here. can't break around the top 
			 */
			if (!overthetop) {
				lbn = 0;
				overthetop++;
			} else {
				growbit(lbn + DDDB_BITBLOCK);
			}
		}
		slbn = lbn;
		tbcnt = bcnt;

		while (1) {
			if ((bitm[lbn >> 3] & (1 << (lbn & 7))) != 0)
				break;

			/*
			 * enough free blocks - mark and done 
			 */
			if (--tbcnt == 0) {
				for (tbcnt = slbn; bcnt > 0; tbcnt++, bcnt--)
					bitm[tbcnt >> 3] |= (1 << (tbcnt & 7));

				if (offend) {
					last_free = 0;
				} else {
					last_free = lbn;
				}
				return (slbn);
			}
			lbn++;
			if (lbn >= bm_top - 32) {
				offend++;
				growbit(lbn + DDDB_BITBLOCK);
			}
		}
		lbn++;
	}
}

Obj *
 dddb_get(nam)
Objname *nam;
{
	Obj *ret;

	if (!db_initted)
		return ((Obj *) 0);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Objname);
	dat = gdbm_fetch(dbp, key);

	if (dat.dptr == (char *)0)
		return ((Obj *) 0);
	bcopy(dat.dptr, (char *)&hbuf, sizeof(hbuf));

	/*
	 * seek to location 
	 */
	if (fseek(dbf, (long)hbuf.off, 0))
		return ((Obj *) 0);

	/*
	 * if the file is badly formatted, ret == Obj * 0 
	 */
	if ((ret = objfromFILE(dbf)) == (Obj *) 0) {
		logf("db_get: cannot decode ", nam, "\n", (char *)0);
		return NULL;
	}
	
#ifndef STANDALONE	
	/* Check to make sure the requested name and stored name are the
	   same */
	
	if (*nam != ret->name) {
		STARTLOG(LOG_ALWAYS, "BUG", "CRUPT")
			log_text((char *)tprintf("Database is corrupt, object %d. Exiting.", (int)ret->name));
		ENDLOG
		raw_broadcast(0, "Game: Database corruption detected, exiting.");
		exit(8);
	}
#endif
		
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

	dat = gdbm_fetch(dbp, key);

	if (dat.dptr != (char *)0) {

		bcopy(dat.dptr, (char *)&hbuf, sizeof(hbuf));	/*
								 * align 
								 */

		if (BLOCKS_NEEDED(nsiz) > hbuf.blox) {

#ifdef	DBMCHUNK_DEBUG
			printf("put: moving obj %d, owned %d blox, required %d, siz %d\n",
			       *(unsigned int *)nam, hbuf.blox, BLOCKS_NEEDED(nsiz), nsiz);
#endif
			/*
			 * mark free in bitmap 
			 */
			dddb_mark(LOGICAL_BLOCK(hbuf.off), hbuf.blox, 0);

			hbuf.off = BLOCK_OFFSET(dddb_alloc(nsiz));
			hbuf.siz = nsiz;
			hbuf.blox = BLOCKS_TO_ALLOC(nsiz);
#ifdef	DBMCHUNK_DEBUG
			printf("put: %s moved to offset %d, size %d\n",
			       *(unsigned int *)nam, hbuf.off, hbuf.siz);
#endif
		} else {
			hbuf.siz = nsiz;
#ifdef	DBMCHUNK_DEBUG
			printf("put: %s replaced within offset %d, size %d owns %d blox\n",
			*(unsigned int *)nam, hbuf.off, hbuf.siz, hbuf.blox);
#endif
		}
	} else {
		hbuf.off = BLOCK_OFFSET(dddb_alloc(nsiz));
		hbuf.siz = nsiz;
		hbuf.blox = BLOCKS_TO_ALLOC(nsiz);
#ifdef	DBMCHUNK_DEBUG
		printf("put: %s (new) at offset %d, size %d owns %d blox\n",
		       *(unsigned int *)nam, hbuf.off, hbuf.siz, hbuf.blox);
#endif
	}


	/*
	 * make table entry 
	 */
	dat.dptr = (char *)&hbuf;
	dat.dsize = sizeof(hbuf);

	if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
		logf("db_put: can't gdbm_store ", nam, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
#ifdef	DBMCHUNK_DEBUG
	printf("%s offset %d size %d owns %d blox\n",
	       *(unsigned int *)nam, hbuf.off, hbuf.siz, hbuf.blox);
#endif
	if (fseek(dbf, (long)hbuf.off, 0)) {
		logf("db_put: can't seek ", nam, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
	if (objtoFILE(obj, dbf) != 0 || fflush(dbf) != 0) {
		logf("db_put: can't save ", nam, " ", (char *)-1, "\n", (char *)0);
		return (1);
	}
	return (0);
}

int dddb_check(nam)
char *nam;
{
	if (!db_initted)
		return (0);

	key.dptr = nam;
	key.dsize = strlen(nam) + 1;

	dat = gdbm_fetch(dbp, key);

	if (dat.dptr == (char *)0)
		return (0);
	return (1);
}

int dddb_del(nam)
Objname *nam;
{

	if (!db_initted)
		return (-1);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Objname);
	dat = gdbm_fetch(dbp, key);

	/*
	 * not there? 
	 */
	if (dat.dptr == (char *)0)
		return (0);
	bcopy(dat.dptr, (char *)&hbuf, sizeof(hbuf));

	/*
	 * drop key from db 
	 */
	if (gdbm_delete(dbp, key)) {
		logf("db_del: can't delete key ", nam, "\n", (char *)0);
		return (1);
	}
	/*
	 * mark free space in bitmap 
	 */
	dddb_mark(LOGICAL_BLOCK(hbuf.off), hbuf.blox, 0);
#ifdef	DBMCHUNK_DEBUG
	printf("del: %s free offset %d, size %d owned %d blox\n",
	       *(unsigned int *)nam, hbuf.off, hbuf.siz, hbuf.blox);
#endif

	/*
	 * mark object dead in file 
	 */
	if (fseek(dbf, (long)hbuf.off, 0))
		logf("db_del: can't seek ", nam, " ", (char *)-1, "\n", (char *)0);
	else {
		fprintf(dbf, "delobj");
		fflush(dbf);
	}
	return (0);
}
