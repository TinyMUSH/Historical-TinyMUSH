/* api.c - functions called only by modules */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by interface */
#include "interface.h"	/* required by code */

#include "command.h"	/* required by code */
#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "udb_defs.h"	/* required by code */
#include "udb.h"	/* required by code */

/* ---------------------------------------------------------------------------
 * Exporting a module's own API.
 */

void register_api(module_name, api_name, ftable)
    char *module_name, *api_name;
    API_FUNCTION *ftable;
{
    MODULE *mp;
    API_FUNCTION *afp;
    void (*fn_ptr)(void *, void *);
    int succ = 0;

    WALK_ALL_MODULES(mp) {
	if (!strcmp(module_name, mp->modname)) {
	    succ = 1;
	    break;
	}
    }
    if (!succ)		/* no such module */
	return;

    for (afp = ftable; afp->name; afp++) { 
	fn_ptr = DLSYM(mp->handle, module_name, afp->name, (void *, void *));
	if (fn_ptr != NULL) {
	    afp->handler = fn_ptr;
	    hashadd(tprintf("%s_%s", api_name, afp->name),
		    (int *) afp, &mudstate.api_func_htab, 0);
	}
    }
}

void *request_api_function(api_name, fn_name)
    char *api_name, *fn_name;
{
    API_FUNCTION *afp;

    afp = (API_FUNCTION *) hashfind(tprintf("%s_%s", api_name, fn_name),
				    &mudstate.api_func_htab);
    if (!afp)
	return NULL;
    return afp->handler;
}

/* ---------------------------------------------------------------------------
 * Handle tables.
 */

void register_commands(cmdtab)
    CMDENT *cmdtab;
{
    CMDENT *cp;

    for (cp = cmdtab; cp->cmdname; cp++)
	hashadd(cp->cmdname, (int *) cp, &mudstate.command_htab, 0);
}

void register_functions(functab)
    FUN *functab;
{
    FUN *fp;

    for (fp = functab; fp->name; fp++) {
	hashadd((char *)fp->name, (int *) fp, &mudstate.func_htab, 0);
    }
}

void register_hashtables(htab, ntab)
    MODHASHES *htab;
    MODNHASHES *ntab;
{
    MODHASHES *hp;
    MODNHASHES *np;

    if (htab) {
	for (hp = htab; hp->tabname != NULL; hp++) {
	    hashinit(hp->htab, hp->size_factor * HASH_FACTOR);
	}
    }

    if (ntab) {
	for (np = ntab; np->tabname != NULL; np++) {
	    nhashinit(np->htab, np->size_factor * HASH_FACTOR);
	}
    }
}

/* ---------------------------------------------------------------------------
 * Deal with additional database info.
 */

unsigned int register_dbtype(modname)
char *modname;
{
	void *data;
	unsigned int type;
	
	/* Find out if the module already has a registered DB type */
	
	dddb_get((void *)modname, strlen(modname) + 1, &data, NULL, DBTYPE_MODULETYPE);

	if (data) {
		memcpy((void *)&type, (void *)data, sizeof(unsigned int));
		XFREE(data, "register_dbtype");
		return type;
	}
	
	/* If the type is in range, return it, else return zero as 
	 * an error code */
	 
	if ((mudstate.moduletype_top >= DBTYPE_RESERVED) &&
	    (mudstate.moduletype_top < DBTYPE_END)) {
		/* Write the entry to GDBM */
		
		dddb_put((void *)modname, strlen(modname) + 1, (void *)&mudstate.moduletype_top, sizeof(unsigned int), DBTYPE_MODULETYPE);
		type = mudstate.moduletype_top;
		mudstate.moduletype_top++;
		return type;
	} else {
		return 0;
	}
}
	
