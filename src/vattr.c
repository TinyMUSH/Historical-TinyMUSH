
/*
 * vattr.c -- Manages the user-defined attributes. 
 */
/*
 * $Id$ 
 */

#include "copyright.h"
#include "autoconf.h"

#include "copyright.h"
#include "mudconf.h"
#include "vattr.h"
#include "alloc.h"
#include "htab.h"
#include "externs.h"

static void FDECL(fixcase, (char *));
static char FDECL(*store_string, (char *));

/*
 * Allocate space for strings in lumps this big. 
 */

#define STRINGBLOCK 1000

/*
 * Current block we're putting stuff in 
 */

static char *stringblock = (char *)0;

/*
 * High water mark. 
 */

static int stringblock_hwm = 0;

void NDECL(vattr_init)
{
	hashinit(&mudstate.vattr_name_htab, VATTR_HASH_SIZE);
}

VATTR *vattr_find(name)
char *name;
{
	register VATTR *vp;

	if (!ok_attr_name(name))
		return (NULL);

	vp = (VATTR *)hashfind(name, &mudstate.vattr_name_htab);
	
	/*
	 * vp is NULL or the right thing. It's right, either way. 
	 */
	return (vp);
}

VATTR *vattr_alloc(name, flags)
char *name;
int flags;
{
	int number;

	if (((number = mudstate.attr_next++) & 0x7f) == 0)
		number = mudstate.attr_next++;
	anum_extend(number);
	return (vattr_define(name, number, flags));
}

VATTR *vattr_define(name, number, flags)
char *name;
int number, flags;
{
	VATTR *vp;

	/*
	 * Be ruthless. 
	 */

	if (strlen(name) >= VNAME_SIZE)
		name[VNAME_SIZE - 1] = '\0';

	fixcase(name);
	if (!ok_attr_name(name))
		return (NULL);

	if ((vp = vattr_find(name)) != NULL)
		return (vp);

	vp = (VATTR *) malloc(sizeof(VATTR));

	vp->name = store_string(name);
	vp->flags = flags;
	vp->number = number;

	hashadd(vp->name, (int *) vp, &mudstate.vattr_name_htab);
	
	anum_extend(vp->number);
	anum_set(vp->number, (ATTR *) vp);
	return (vp);
}

void do_dbclean(player, cause, key)
dbref player, cause;
int key;
{
#ifndef STANDALONE

    VATTR *vp, *vpx;
    dbref i;
    int ca;
    char *as;
    int *used_table;

    raw_broadcast(0,
	      "GAME: Cleaning database. Game may freeze for a few minutes.");

    used_table = (int *) calloc(mudstate.attr_next, sizeof(int));

    /* Walk the database. Mark all the attribute numbers in use. */

    DO_WHOLE_DB(i) {
	for (ca = atr_head(i, &as); ca; ca = atr_next(&as)) {
	    used_table[ca] = 1;
	}
#ifndef MEMORY_BASED
	if ((i % 100) == 0) 
	    cache_reset(0);
#endif
    }

    /* Walk the vattr table. If a number isn't in use, zorch it. */

    vp = vattr_first();
    while (vp) {
	vpx = vp;
	vp = vattr_next(vp);
	if (used_table[vpx->number] == 0) {
	    anum_set(vpx->number, NULL);
	    hashdelete(vpx->name, &mudstate.vattr_name_htab);
	    free((VATTR *) vpx);
	}
    }

    free(used_table);

    raw_broadcast(0, "GAME: Database cleaning complete.");

#endif /* STANDALONE */
}
		
void vattr_delete(name)
char *name;
{
	VATTR *vp;
	int number;

	fixcase(name);
	if (!ok_attr_name(name))
		return;

	number = 0;

	vp = (VATTR *)hashfind(name, &mudstate.vattr_name_htab);
	
	if (vp) {
		number = vp->number;
		anum_set(number, NULL);
		hashdelete(name, &mudstate.vattr_name_htab);
		free((char *)vp);
	}
	
	return;
}

VATTR *vattr_rename(name, newname)
char *name, *newname;
{
	VATTR *vp;

	fixcase(name);
	if (!ok_attr_name(name))
		return (NULL);

	/*
	 * Be ruthless. 
	 */

	if (strlen(newname) >= VNAME_SIZE)
		newname[VNAME_SIZE - 1] = '\0';

	fixcase(newname);
	if (!ok_attr_name(newname))
		return (NULL);
	
	/* We must explicitly delete and add the name to the hashtable,
	 * since we are changing the data.
	 */

	vp = (VATTR *)hashfind(name, &mudstate.vattr_name_htab);

	if (vp) {
	    vp->name = store_string(newname);
	    hashdelete(name, &mudstate.vattr_name_htab);
	    hashadd(newname, (int *) vp, &mudstate.vattr_name_htab);
	}

	return (vp);
}

VATTR *NDECL(vattr_first)
{
	return (VATTR *)hash_firstentry(&mudstate.vattr_name_htab);
}

VATTR *vattr_next(vp)
VATTR *vp;
{
	if (vp == NULL)
		return (vattr_first());

	return ((VATTR *)hash_nextentry(&mudstate.vattr_name_htab));
}

static void fixcase(name)
char *name;
{
	char *cp = name;

	while (*cp) {
		*cp = ToUpper(*cp);
		cp++;
	}

	return;
}


/*
 * Some goop for efficiently storing strings we expect to
 * keep forever. There is no freeing mechanism.
 */

static char *store_string(str)
char *str;
{
	int len;
	char *ret;

	len = strlen(str);

	/*
	 * If we have no block, or there's not enough room left in the * * *
	 * current one, get a new one. 
	 */

	if (!stringblock || (STRINGBLOCK - stringblock_hwm) < (len + 1)) {
		stringblock = (char *)malloc(STRINGBLOCK);
		if (!stringblock)
			return ((char *)0);
		stringblock_hwm = 0;
	}
	ret = stringblock + stringblock_hwm;
	StringCopy(ret, str);
	stringblock_hwm += (len + 1);
	return (ret);
}
