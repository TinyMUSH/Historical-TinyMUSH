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
