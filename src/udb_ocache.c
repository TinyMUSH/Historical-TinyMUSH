/* udb_ocache.c - LRU attribute caching */ 
/* $Id$ */

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

extern void VDECL(logf, (char *, ...));
extern void VDECL(fatal, (char *, ...));
extern void FDECL(log_db_err, (int, int, const char *));

extern void FDECL(raw_notify, (dbref, const char *));

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

/* Set last referenced time to zero. This means that we're willing to throw
 * this cache entry away anytime
 */

#define CLRREFTIME(cp)	cp->lastreferenced = 0;

static Cache *get_free_entry();
static int cache_write();
static void cache_clean();

/* initial settings for cache sizes */

static int cwidth = CACHE_WIDTH;

/* sys_c points to all cache lists, active and modified */

static CacheLst *sys_c;

static int cache_initted = 0;
static int cache_frozen = 0;
static int cache_busy = 0;

/* cache stats */

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

int cache_init(width)
int width;
{
	int x;
	CacheLst *sp;
	static char *ncmsg = "cache_init: cannot allocate cache: ";

	if (cache_initted || sys_c != (CacheLst *) 0)
		return (0);

	/*
	 * If either dimension is specified as non-zero, change it to that,
	 * otherwise use default. Treat dimensions deparately.
	 */

	if (width)
		cwidth = width;

	sp = sys_c = (CacheLst *) XMALLOC((unsigned)cwidth * sizeof(CacheLst), "cache_init");
	if (sys_c == (CacheLst *) 0) {
		logf(ncmsg, (char *)-1, "\n", (char *)0);
		return (-1);
	}

	/* Allocate the initial cache entries */

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

void cache_reset()
{
	int x;
	Cache *cp, *nxt;
	CacheLst *sp;

	/* Clear the cache after startup and reset stats */

	for (x = 0; x < cwidth; x++, sp++) {
		sp = &sys_c[x];
	
		/* traverse active chain first */
		for (cp = sp->active.head; cp != NULL; cp = nxt) {
			nxt = cp->nxt;
			
			cache_repl(cp, NULL);
			XFREE(cp, "cache_reset.act");
		}
		
		/* then the modified active chain */
		for (cp = sp->mactive.head; cp != NULL; cp = nxt) {
			nxt = cp->nxt;
			
			if (cp->op == NULL) {
				(void) ATTR_DEL(&(cp->onm));
			} else {
				(void) ATTR_PUT(&(cp->onm), cp->op);
			}
			cache_repl(cp, NULL);
			XFREE(cp, "cache_reset.mact");
		}
		
		sp->active.head = (Cache *) 0;
		sp->active.tail = (Cache *) 0;
		sp->mactive.head = (Cache *) 0;
		sp->mactive.tail = (Cache *) 0;
	}
	
	/* Clear the counters after startup, or they'll be skewed */
	
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
	cs_syncs = 0;		/* total cache syncs */
	cs_size = 0;		/* size of cache in bytes */
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

/* Search the cache for an attribute, if found, return, if not, fetch from
 * DB
 */

Attr *cache_get(nam)
Aname *nam;
{
	Cache *cp;
	CacheLst *sp;
	int hv = 0;
	Attr *ret;

	if (nam == (Aname *) 0 || !cache_initted) {
		return ((Attr *) 0);
	}

	/* If we're dumping, ignore stats - activity during a dump skews the
	 * working set. We make sure in get_free_entry that any activity
	 * resulting from a dump does not push out entries that are already
	 * in the cache */

#ifndef STANDALONE
	if (!mudstate.dumping)
		cs_reads++;
#endif

	hv = (nam->object + nam->attrnum) % cwidth;
	sp = &sys_c[hv];

	/* search active chain first */
	
	for (cp = sp->active.head; cp != NULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
#ifndef STANDALONE
			if (!mudstate.dumping) {
				cs_rhits++;
				cs_ahits++;
			}
#endif
			DEQUEUE(sp->active, cp);
			INSHEAD(sp->active, cp);

			REFTIME(cp);

			return (cp->op);
		}
	}

	/* search modified active chain next. */
	
	for (cp = sp->mactive.head; cp != NULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
#ifndef STANDALONE
			if (!mudstate.dumping) {
				cs_rhits++;
				cs_ahits++;
			}
#endif
			DEQUEUE(sp->mactive, cp);
			INSHEAD(sp->mactive, cp);

			REFTIME(cp);
			return (cp->op);
		}
	}

	/* DARN IT - at this point we have a certified, type-A cache miss */

	if ((ret = ATTR_GET(nam)) == NULL) {
#ifndef STANDALONE
		if (!mudstate.dumping)
			cs_dbreads++;
#endif
		return (NULL);
	} 
#ifndef STANDALONE	
	else {
		if (!mudstate.dumping)
			cs_dbreads++;
	}
#endif

	if ((cp = get_free_entry(strlen((char *)ret))) == NULL)
		return (NULL);

	cp->onm = *nam;
	cp->op = ret;

	/* If we're dumping, we'll put everything we fetch that is not
	   already in cache at the end of the chain and set its last
	   referenced time to zero. This will ensure that we won't blow away
	   what's already in cache, since get_free_entry will just reuse
	   these entries. */
	
	cp->size = strlen((char *)ret);
	cs_size += cp->size;

#ifndef STANDALONE
	if (mudstate.dumping) {
		/* Link at tail of active chain */
		INSTAIL(sp->active, cp);
		CLRREFTIME(cp);
	} else {
#endif
		/* Link at head of active chain */
		INSHEAD(sp->active, cp);
		REFTIME(cp);
#ifndef STANDALONE
	}
#endif
	
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
 * so - we do a couple of things: we make sure that the cached object is
 * actually there, and move it to the modified chain. if we can't find it -
 * either we have a (major) programming error, or the
 * *name* of the object has been changed, or the object is a totally
 * new creation someone made and is inserting into the world.
 * 
 * in the case of totally new creations, we simply accept the pointer
 * to the object and add it into our environment. freeing it becomes
 * the responsibility of the cache code. DO NOT HAND A POINTER TO
 * CACHE_PUT AND THEN FREE IT YOURSELF!!!!
 * 
 */

int cache_put(nam, obj)
Aname *nam;
Attr *obj;
{
	Cache *cp;
	CacheLst *sp;
	int hv = 0;
	
	if (obj == (Attr *) 0 || nam == (Aname *) 0 || !cache_initted) {
		return (1);
	}
	cs_writes++;

	/* generate hash */
	
	hv = (nam->object + nam->attrnum) % cwidth;
	sp = &sys_c[hv];

	/* step one, search active chain, and if we find the obj, dirty it */
	
	for (cp = sp->active.head; cp != NULL; cp = cp->nxt) {
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
	for (cp = sp->mactive.head; cp != NULL; cp = cp->nxt) {
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

	if ((cp = get_free_entry(strlen((char *)obj))) == NULL)
		return (1);

	cp->op = obj;
	cp->onm = *nam;
	cp->size = strlen((char *)obj);
	cs_size += cp->size;

	/* link at head of modified active chain */
	
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
	
	/* Flush attributes from the cache until there's enough room for
	 * this one. The max size can be dynamically changed-- if it is too
	 * small, the MUSH will flush objects until the cache fits within
	 * this size and if it is too large, we'll fill it up before we
	 * start flushing */
	
	while ((cs_size + atrsize) > 
		(mudconf.cache_size ? mudconf.cache_size : CACHE_SIZE)) {
		for (x = 0; x < cwidth; x++) {
			sp = &sys_c[x];
	
			p = sp->active.tail;
		
			/* Score is the age of an attribute in seconds.
			   We use size as a secondary metric-- if the scores
			   are the same, we should try to toss the bigger
			   attribute. Only consider the tail of each chain
			   since we re-insert each cache entry at the head
			   when it's accessed */

			if (p) {
				/* Automatically toss this bucket if it is
				   empty or lastreferenced is zero (which
				   means we don't want to keep it) */
				   
				if (!p->size || !p->lastreferenced) {
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
				/* Automatically toss this bucket if it is
				   empty or lastreferenced is zero (which
				   means we don't want to keep it) */

				if (!p->size || !p->lastreferenced) {
					cp = p;
					chp = &(sp->mactive);
					modified = 1;
					goto replace;
				} else {
					/* We don't want to prematurely toss
					 * modified pages, so give them an
					 * advantage by lowering their score
					 */
	
#ifndef STANDALONE 
					score = (mudstate.now - p->lastreferenced) * .8;
#else
					score = (time(NULL) - p->lastreferenced) * .8;
#endif
					size = p->size;
				}
						
				/* If we haven't found one by now, the tail
				 * of the modified chain is it
				 */

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
				if (ATTR_DEL(&(cp->onm))) {
					log_db_err(cp->onm.object, cp->onm.attrnum, "delete");
					return (NULL);
				}
				cs_dels++;
			} else {
				if (ATTR_PUT(&(cp->onm), cp->op)) {
					log_db_err(cp->onm.object, cp->onm.attrnum, "write");
					return (NULL);
				}
				cs_dbwrites++;
			}
		}
		
		/* Take the attribute off of its chain and nuke the
		   attribute's memory */
		
		if (cp) {
			cache_repl(cp, NULL);
			DEQUEUE((*chp), cp);
			XFREE(cp, "get_free_entry");
		}
		cp = NULL;
	}		

	/* No valid cache entries to flush, allocate a new one */

	if ((cp = (Cache *) XMALLOC(sizeof(Cache), "get_free_entry")) == NULL)
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
	/* Write a single cache chain to disk */

	while (cp != NULL) {
		if (cp->op == NULL) {
			if (ATTR_DEL(&(cp->onm))) {
				log_db_err(cp->onm.object, cp->onm.attrnum, "delete");
				return (1);
			}
			cs_dels++;
		} else {
			if (ATTR_PUT(&(cp->onm), cp->op)) {
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
	/* move modified active chain to the active chain */
	
	if (sp->mactive.head != NULL) {
		if (sp->active.head == NULL) {
			sp->active.head = sp->mactive.head;
			sp->active.tail = sp->mactive.tail;
		} else {
			sp->active.tail->nxt = sp->mactive.head;
			sp->mactive.head->prv = sp->active.tail;
			sp->active.tail = sp->mactive.tail;
		}
		sp->mactive.head = sp->mactive.tail = NULL;
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

	for (x = 0, sp = sys_c; x < cwidth; x++, sp++) {
		if (cache_write(sp->mactive.head))
			return (1);
		cache_clean(sp);
	}
	
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

	/* mark dead in cache */
	
	for (cp = sp->active.head; cp != NULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			DEQUEUE(sp->active, cp);
			INSTAIL(sp->mactive, cp);
			cache_repl(cp, NULL);
			REFTIME(cp);
			return;
		}
	}
	for (cp = sp->mactive.head; cp != NULL; cp = cp->nxt) {
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

