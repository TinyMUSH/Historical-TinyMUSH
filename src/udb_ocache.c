/* udb_ocache.c - modified untermud object cache */ 
/* $Id$ */

/*
 * Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
 */

/*
 * #define CACHE_DEBUG
 * #define CACHE_VERBOSE
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
#include "udb_defs.h"

/* Need the database environment pointer to do checkpoints */

extern DB_ENV *dbenvp;

extern Attr * FDECL(dddb_get, (Aname *));
extern void VDECL(logf, (char *, ...));
extern int FDECL(dddb_del, (Aname *));
extern int FDECL(dddb_put, (Attr *, Aname *));
extern void VDECL(fatal, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

extern void FDECL(raw_notify, (dbref, const char *));

/*
 * This is by far the most complex and kinky code in UnterMUD. You should
 * never need to mess with anything in here - if you value your sanity.
 */


typedef struct cache {
	Aname onm;
	Attr *op;
	int size;
	int referenced;
	time_t lastreferenced;
	struct cache *nxt;
	struct cache *prv;
} Cache;

typedef struct {
	Cache *head;
	Cache *tail;
} Chain;

typedef struct {
	Chain active;
	Chain mactive;
} CacheLst;

#define NAMECMP(a,b)	((a)->onm.object == (b)->object) && \
		    	((a)->onm.attrnum == (b)->attrnum)

#define DEQUEUE(q, e)	if(e->nxt == (Cache *)0) { \
				if (e->prv != (Cache *)0) { \
					e->prv->nxt = (Cache *)0; \
				} \
				q.tail = e->prv; \
			} else { \
				e->nxt->prv = e->prv; \
			} \
			if(e->prv == (Cache *)0) { \
				if (e->nxt != (Cache *)0) { \
					e->nxt->prv = (Cache *)0; \
				} \
				q.head = e->nxt; \
			} else \
				e->prv->nxt = e->nxt;

#define INSHEAD(q, e)	if (q.head == (Cache *)0) { \
				q.tail = e; \
				e->nxt = (Cache *)0; \
			} else { \
				q.head->prv = e; \
				e->nxt = q.head; \
			} \
			q.head = e; \
			e->prv = (Cache *)0;

#define INSTAIL(q, e)	if (q.head == (Cache *)0) { \
				q.head = e; \
				e->prv = (Cache *)0; \
			} else { \
				q.tail->nxt = e; \
				e->prv = q.tail; \
			} \
			q.tail = e; \
			e->nxt = (Cache *)0;

#ifndef STANDALONE
			/* If the object has been referenced within the last
			 * queue cycle, ignore it-- else increment the counter
			 */
#define REFTIME(cp)	if (mudstate.now != cp->lastreferenced) { \
				cp->referenced++; \
				cp->lastreferenced = mudstate.now; \
			}
#else /* STANDALONE */
			/* If the object has been referenced within the last
			 * two seconds, ignore it-- else increment the counter
			 */
#define REFTIME(cp)	if ((time(NULL) - cp->lastreferenced) > 2) { \
				cp->referenced++; \
				cp->lastreferenced = time(NULL); \
			}
#endif /* STANDALONE */

static Cache *get_free_entry();
static int cache_write();
static void cache_clean();

/*
 * initial settings for cache sizes 
 */
static int cwidth = CACHE_WIDTH;
static int cmaxsize = CACHE_SIZE;

/*
 * ntbfw - main cache pointer and list of things to kill off 
 */
static CacheLst *sys_c;

static int cache_initted = 0;
static int cache_frozen = 0;
static int cache_busy = 0;

/*
 * cache stats gathering stuff. you don't like it? comment it out 
 */
time_t cs_ltime;
int cs_writes = 0;		/* total writes */
int cs_reads = 0;		/* total reads */
int cs_dbreads = 0;		/* total read-throughs */
int cs_dbwrites = 0;		/* total write-throughs */
int cs_dels = 0;		/* total deletes */
int cs_checks = 0;		/* total checks */
int cs_rhits = 0;		/* total reads filled from cache */
int cs_ahits = 0;		/* total reads filled active cache */
int cs_whits = 0;		/* total writes to dirty cache */
int cs_fails = 0;		/* attempts to grab nonexistent */
int cs_resets = 0;		/* total cache resets */
int cs_syncs = 0;		/* total cache syncs */
int cs_size = 0;		/* total cache size */

void cache_repl(cp, new)
Cache *cp;
Attr *new;
{
	cs_size -= cp->size;
	if (cp->op != NULL)
		XFREE(cp->op, "cache_repl");
	cp->op = new;
	if (new != NULL) {
		cp->size = strlen(new);
		cs_size += cp->size;
	} else {
		cp->size = 0;
	}
}

int cache_init(width, size)
int width;
int size;
{
	int x;
	CacheLst *sp;
	static char *ncmsg = "cache_init: cannot allocate cache: ";

	if (cache_initted || sys_c != (CacheLst *) 0)
		return (0);

	/*
	 * If either dimension is specified as non-zero, change it to 
	 * that, otherwise use default. Treat dimensions deparately.  
	 */

	if (width)
		cwidth = width;
	if (size)
		cmaxsize = size;

	sp = sys_c = (CacheLst *) XMALLOC((unsigned)cwidth * sizeof(CacheLst), "cache_init");
	if (sys_c == (CacheLst *) 0) {
		logf(ncmsg, (char *)-1, "\n", (char *)0);
		return (-1);
	}

	/*
	 * Allocate the initial cache entries
	 */

	for (x = 0; x < cwidth; x++, sp++) {
		sp->active.head = (Cache *) 0;
		sp->active.tail = (Cache *) 0;
		sp->mactive.head = (Cache *) 0;
		sp->mactive.tail = (Cache *) 0;
	}

	/* mark caching system live */
	cache_initted++;

	cs_ltime = time(NULL);

	return (0);
}

void cache_reset(clear)
int clear;
{
	int x;
	Cache *cp, *nxt;
	CacheLst *sp;

	/* Implement cache aging by decrementing the reference counter on each object */

	for (x = 0; x < cwidth; x++, sp++) {
		sp = &sys_c[x];
	
		/* traverse active chain first */
		for (cp = sp->active.head; cp != CNULL; cp = nxt) {
			nxt = cp->nxt;
			
			if (clear) {
				cache_repl(cp, NULL);
				XFREE(cp, "cache_reset.act");
			} else {
				if (cp->referenced > 0)
					cp->referenced--;
			}
		}
		
		/* then the modified active chain */
		for (cp = sp->mactive.head; cp != CNULL; cp = nxt) {
			nxt = cp->nxt;
			
			if (clear) {
				if (cp->op == NULL) {
					(void) DB_DEL(&(cp->onm));
				} else {
					(void) DB_PUT(cp->op, &(cp->onm));
				}
				cache_repl(cp, NULL);
				XFREE(cp, "cache_reset.mact");
			} else {
				if (cp->referenced > 0)
					cp->referenced--;
			}
		}
		
		if (clear) {
			sp->active.head = (Cache *) 0;
			sp->active.tail = (Cache *) 0;
			sp->mactive.head = (Cache *) 0;
			sp->mactive.tail = (Cache *) 0;
		}
	}
	
	/* Clear the counters after startup, or they'll be skewed */
	
	if (clear) {
		cs_writes = 0;		/* total writes */
		cs_reads = 0;		/* total reads */
		cs_dbreads = 0;		/* total read-throughs */
		cs_dbwrites = 0;	/* total write-throughs */
		cs_dels = 0;		/* total deletes */
		cs_checks = 0;		/* total checks */
		cs_rhits = 0;		/* total reads filled from cache */
		cs_ahits = 0;		/* total reads filled active cache */
		cs_whits = 0;		/* total writes to dirty cache */
		cs_fails = 0;		/* attempts to grab nonexistent */
		cs_resets = 0;		/* total cache resets */
		cs_syncs = 0;		/* total cache syncs */
		cs_size = 0;		/* size of cache in bytes */
	}
}

#ifndef STANDALONE
/* list dbrefs of objects in the cache. */

void list_cached_objs(player)
    dbref player;
{
    CacheLst *sp;
    Cache *cp;
    int x;
    int aco, maco, asize, msize;
    ATTR *atr;
    
    aco = maco = asize = msize = 0;

    raw_notify(player, "Active Cache:");
    raw_notify(player, 
       "Name                    Attribute               Dbref   Age    Refs   Size");
    raw_notify(player,
       "==========================================================================");
    for (x = 0, sp = sys_c; x < cwidth; x++, sp++) {
        for (cp = sp->active.head; cp != NULL; cp = cp->nxt) {
            if (cp->op) {
                aco++;
                asize += cp->size;
                atr = atr_num(cp->onm.attrnum);
		raw_notify(player, 
			tprintf("%-23.23s %-23.23s #%-6d %-6d %-6d %-6d", PureName(cp->onm.object),
			(atr ? atr->name : "(Unknown)"), cp->onm.object, (int) (mudstate.now - cp->lastreferenced), 
			cp->referenced, cp->size));
            }
        }
    }

    raw_notify(player, "\nModified Active Cache:");
    raw_notify(player, 
       "Name                    Attribute               Dbref   Age    Refs   Size");
    raw_notify(player,
       "==========================================================================");
    for (x = 0, sp = sys_c; x < cwidth; x++, sp++) {
        for (cp = sp->mactive.head; cp != NULL; cp = cp->nxt) {
            if (cp->op) {
                aco++;
                asize += cp->size;
                atr = atr_num(cp->onm.attrnum);
		raw_notify(player, 
			tprintf("%-23.23s %-23.23s #%-6d %-6d %-6d %-6d", PureName(cp->onm.object),
			(atr ? atr->name : "(Unknown)"), cp->onm.object, (int) (mudstate.now - cp->lastreferenced), 
			cp->referenced, cp->size));
            }
        }
    }

    raw_notify(player,
               tprintf("\nTotals: active %d, modified active %d, total attributes %d", aco, maco, aco + maco));
    raw_notify(player,
               tprintf("Size: active %d bytes, modified active %d bytes", asize, msize));
}
#endif /* STANDALONE */

/*
 * Search the cache for an attribute, if found, return, if not, fetch from DB
 */
Attr * cache_get(nam)
Aname *nam;
{
	Cache *cp;
	CacheLst *sp;
	int hv = 0;
	Attr *ret;

	/*
	 * firewall 
	 */
	if (nam == (Aname *) 0 || !cache_initted) {
#ifdef	CACHE_VERBOSE
		logf("cache_get: NULL object name - programmer error\n", (char *)0);
#endif
		return ((Attr *) 0);
	}
#ifdef	CACHE_DEBUG
	printf("get %d/%d\n", nam->object, nam->attrnum);
#endif

	cs_reads++;

	hv = (nam->object + nam->attrnum) % cwidth;
	sp = &sys_c[hv];

	/*
	 * search active chain first 
	 */
	for (cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			cs_rhits++;
			cs_ahits++;

			DEQUEUE(sp->active, cp);
			INSHEAD(sp->active, cp);

			REFTIME(cp);

			return (cp->op);
		}
	}

	/*
	 * search modified active chain next. 
	 */
	for (cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			cs_rhits++;
			cs_ahits++;

			DEQUEUE(sp->mactive, cp);
			INSHEAD(sp->mactive, cp);

			REFTIME(cp);
			return (cp->op);
		}
	}

	/*
	 * DARN IT - at this point we have a certified, type-A cache miss 
	 */

	if ((ret = DB_GET(nam)) == NULL) {
		cs_dbreads++;
		return (NULL);
	} else {
		cs_dbreads++;
	}

	if ((cp = get_free_entry(strlen((char *)ret))) == NULL)
		return (NULL);

	cp->onm = *nam;
	cp->op = ret;
	cp->size = strlen((char *)ret);
	cs_size += cp->size;

	/*
	 * relink at head of active chain 
	 */
	INSHEAD(sp->active, cp);

	REFTIME(cp);
	return (ret);
}

/*
 * put an attribute back into the cache. this is complicated by the
 * fact that when a function calls this with an object, the object
 * is *already* in the cache, since calling functions operate on
 * pointers to the cached objects, and may or may not be actively
 * modifying them. in other words, by the time the object is handed
 * to cache_put, it has probably already been modified, and the cached
 * version probably already reflects those modifications!
 * 
 * so - we do a couple of things: we make sure that the cached
 * object is actually there, and set its dirty bit. if we can't
 * find it - either we have a (major) programming error, or the
 * *name* of the object has been changed, or the object is a totally
 * new creation someone made and is inserting into the world.
 * 
 * in the case of totally new creations, we simply accept the pointer
 * to the object and add it into our environment. freeing it becomes
 * the responsibility of the cache code. DO NOT HAND A POINTER TO
 * CACHE_PUT AND THEN FREE IT YOURSELF!!!!
 * 
 * There are other sticky issues about changing the object pointers
 * of MUDs and their names. This is left as an exercise for the
 * reader.
 */
int cache_put(nam, obj)
Aname *nam;
Attr *obj;
{
	Cache *cp;
	CacheLst *sp;
	int hv = 0;
	
	if (obj == (Attr *) 0 || nam == (Aname *) 0 || !cache_initted) {

#ifdef	CACHE_VERBOSE
		logf("cache_put: NULL object/name - programmer error\n", (char *)0);
#endif
		return (1);
	}
	cs_writes++;

	/*
	 * generate hash 
	 */
	hv = (nam->object + nam->attrnum) % cwidth;
	sp = &sys_c[hv];

	/*
	 * step one, search active chain, and if we find the obj, dirty it 
	 */
	for (cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if(cp->op != obj) {
				cache_repl(cp, obj);
			}

			DEQUEUE(sp->active, cp);
			INSHEAD(sp->mactive, cp);
			REFTIME(cp);
			cs_whits++;
			return (0);
		}
	}

	/*
	 * step two, search modified active chain, and if we find the obj,
	 * we're done 
	 */
	for (cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if (cp->op != obj) {
				cache_repl(cp, obj);
			}

			DEQUEUE(sp->mactive, cp);
			INSHEAD(sp->mactive, cp);
			
			REFTIME(cp);
			cs_whits++;
			return (0);
		}
	}

	/* Add a new attribute to the cache */

	if ((cp = get_free_entry(strlen((char *)obj))) == CNULL)
		return (1);

	cp->op = obj;
	cp->onm = *nam;
	cp->size = strlen((char *)obj);
	cs_size += cp->size;

	/*
	 * link at head of modified active chain 
	 */
	INSHEAD(sp->mactive, cp);

	REFTIME(cp);
	return (0);
}

static Cache *get_free_entry(atrsize)
int atrsize;
{
	CacheLst *sp;
	Chain *chp;
	Cache *cp = NULL, *p;
	int score = 0, curscore = 0;
	int modified = 0, size = 0, cursize = 0, x;
	
	/* Flush attributes from the cache until there's enough room for this one */
	
	while ((cs_size + atrsize) > cmaxsize) {
		for (x = 0; x < cwidth; x++) {
			sp = &sys_c[x];
	
			p = sp->active.tail;
		
			/* Score is the age of an attribute in seconds.
			   We use size as a secondary metric--
			   if the scores are the same, we should try to
			   toss the bigger attribute. Only consider the
			   head of each chain since we re-insert each
			   cache entry at the head when its accessed */

			if (p) {
				if (!p->size) {
					cp = p;
					chp = &(sp->active);
					modified = 0;
					goto replace;
				} else {
#ifndef STANDALONE
					score = mudstate.now - p->lastreferenced;
#else
					score = time(NULL) - p->lastreferenced;
#endif
					size = p->size;
				}
				
				if ((score > curscore) || ((score == curscore) && 
				    (size > cursize)) || !cp) {
					curscore = score;
					cursize = size;
					cp = p;
					chp = &(sp->active);
					modified = 0;
				}
			}
			
			/* then the modified active chain */

			p = sp->mactive.tail;
			
			if (p) {
				if (!p->size) {
					cp = p;
					chp = &(sp->mactive);
					modified = 1;
					goto replace;
				} else {
					/* We don't want to prematurely toss modified pages, so
					 * give them an advantage */
	
#ifndef STANDALONE 
					score = (mudstate.now - p->lastreferenced) * .8;
#else
					score = (time(NULL) - p->lastreferenced) * .8;
#endif
					size = p->size;
				}
						
				/* If we haven't found one by now, the tail of the modified chain is it */
				if ((score > curscore) || ((score == curscore) && 
				    (size > cursize)) ||
				    ((p == sp->mactive.tail) && !cp)) {
					curscore = score;
					cursize = size;
					cp = p;
					chp = &(sp->mactive);
					modified = 1;
				}
			}
		}
replace:
		if (modified) {
			/* Flush the modified attributes to disk */
		
			if (cp->op == NULL) {
				if (DB_DEL(&(cp->onm))) {
					log_db_err(cp->onm.object, cp->onm.attrnum, "delete");
					return (NULL);
				}
				cs_dels++;
			} else {
				if (DB_PUT(cp->op, &(cp->onm))) {
					log_db_err(cp->onm.object, cp->onm.attrnum, "write");
					return (NULL);
				}
				cs_dbwrites++;
			}
		}
		
		/* Take the attribute off of its chain and nuke the
		   attribute's memory */
		
		if (cp) {
			cache_repl(cp, ONULL);
			DEQUEUE((*chp), cp);
			XFREE(cp, "get_free_entry");
		}
		cp = NULL;
	}		

	/* Just allocate a new one */

	if ((cp = (Cache *) XMALLOC(sizeof(Cache), "get_free_entry")) == CNULL)
		fatal("cache get_free_entry: malloc failed", (char *)-1, (char *)0);

	cp->op = NULL;
	cp->onm.object = 0;
	cp->onm.attrnum = 0;
	cp->size = 0;
	cp->referenced = 1;
#ifndef STANDALONE
	cp->lastreferenced = mudstate.now;
#else
	cp->lastreferenced = time(NULL);
#endif
	return (cp);
}

static int cache_write(cp)
Cache *cp;
{
	while (cp != CNULL) {
#ifdef	CACHE_DEBUG
		printf("sync %d -- %d\n", cp->op->name, cp->op);
#endif
		if (cp->op == NULL) {
			if (DB_DEL(&(cp->onm))) {
				log_db_err(cp->onm.object, cp->onm.attrnum, "delete");
				return (1);
			}
			cs_dels++;
		} else {
			if (DB_PUT(cp->op, &(cp->onm))) {
				log_db_err(cp->onm.object, cp->onm.attrnum, "write");
				return (1);
			}
			cs_dbwrites++;
		}
		cp = cp->nxt;
	}
	return (0);
}

static void cache_clean(sp)
CacheLst *sp;
{
	/*
	 * move modified active chain to the active chain
	 */
	if (sp->mactive.head != CNULL) {
		if (sp->active.head == CNULL) {
			sp->active.head = sp->mactive.head;
			sp->active.tail = sp->mactive.tail;
		} else {
			sp->active.tail->nxt = sp->mactive.head;
			sp->mactive.head->prv = sp->active.tail;
			sp->active.tail = sp->mactive.tail;
		}
		sp->mactive.head = sp->mactive.tail = CNULL;
	}
}

int NDECL(cache_sync)
{
	int x, ret;
	CacheLst *sp;

	cs_syncs++;

	if (!cache_initted)
		return (1);

	if (cache_frozen)
		return (0);

#if 0
	/* This ages the cache by decrementing the reference counter-- useful
	   for a NFU cache, but since we're running an LRU cache, it's not 
	   needed */
	   
	cache_reset(0);
#endif
	for (x = 0, sp = sys_c; x < cwidth; x++, sp++) {
		if (cache_write(sp->mactive.head))
			return (1);
		cache_clean(sp);
	}
	
#ifndef STANDALONE
	/* Checkpoint the underlying DBM database */
	
	if ((ret = txn_checkpoint(dbenvp, 0, 0, 0)) != 0) {
		dbenvp->err(dbenvp, ret, "txn_checkpoint");
		return(1);
	}
#endif
	return (0);
}

void cache_del(nam)
Aname *nam;
{
	Cache *cp;
	CacheLst *sp;
	Obj *obj;
	int hv = 0;
	
	if (nam == (Aname *) 0 || !cache_initted)
		return;

	cs_dels++;

	hv = (nam->object + nam->attrnum) % cwidth;
	sp = &sys_c[hv];

	/*
	 * mark dead in cache 
	 */
	for (cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			DEQUEUE(sp->active, cp);
			INSTAIL(sp->mactive, cp);
			cache_repl(cp, NULL);
			REFTIME(cp);
			return;
		}
	}
	for (cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			DEQUEUE(sp->mactive, cp);
			INSTAIL(sp->mactive, cp);
			cache_repl(cp, NULL);
			REFTIME(cp);
			return;
		}
	}

	if ((cp = get_free_entry(0)) == NULL)
		return;

	cp->op = NULL;
	cp->onm = *nam;
	cp->size = 0;

	REFTIME(cp);
	INSTAIL(sp->mactive, cp);
	return;
}

