/* api.c - functions called only by modules */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#ifdef HAVE_DLOPEN

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mail.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by interface */
#include "interface.h"	/* required by code */

#include "command.h"	/* required by code */
#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */


void register_commands(cmdtab)
    CMDENT *cmdtab;
{
    CMDENT *cp;

    for (cp = cmdtab; cp->cmdname; cp++)
	hashadd(cp->cmdname, (int *) cp, &mudstate.command_htab);
}

#endif /* HAVE_DLOPEN */

