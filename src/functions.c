/* functions.c - MUSH function handlers */
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
#include "externs.h"	/* required by interface */

#include "command.h"	/* required by code */
#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "fnproto.h"	/* required by code */

UFUN *ufun_head;

extern char space_buffer[LBUF_SIZE];

extern void FDECL(cf_log_notfound, (dbref player, char *cmd,
				    const char *thingname, char *thing));

void NDECL(init_functab)
{
	FUN *fp;
	char *buff, *cp, *dp;
	int i;

	buff = alloc_sbuf("init_functab");
	hashinit(&mudstate.func_htab, 250 * HASH_FACTOR);
	for (fp = flist; fp->name; fp++) {
		cp = (char *)fp->name;
		dp = buff;
		while (*cp) {
			*dp = ToLower(*cp);
			cp++;
			dp++;
		}
		*dp = '\0';
		hashadd(buff, (int *)fp, &mudstate.func_htab);
	}
	free_sbuf(buff);
	ufun_head = NULL;
	hashinit(&mudstate.ufunc_htab, 15 * HASH_FACTOR);

	/* Initialize the space table, which is used for fast copies on
	 * functions like ljust().
	 */
	for (i = 0; i < LBUF_SIZE; i++)
	    space_buffer[i] = ' ';
	space_buffer[LBUF_SIZE - 1] = '\0';
}

void do_function(player, cause, key, fname, target)
dbref player, cause;
int key;
char *fname, *target;
{
	UFUN *ufp, *ufp2;
	ATTR *ap;
	char *np, *bp;
	int atr, aflags;
	dbref obj, aowner;

	/* Check for list first */

	if (key & FN_LIST) {
	    
	    if (fname && *fname) {

		/* Make it case-insensitive, and look it up. */

		for (bp = fname; *bp; bp++) {
		    *bp = ToUpper(*bp);
		}

		ufp = (UFUN *) hashfind(fname, &mudstate.ufunc_htab);

		if (ufp) {
		    notify(player,
			   tprintf("%s: #%d/%s",
				   ufp->name, ufp->obj, 
				   ((ATTR *) atr_num(ufp->atr))->name));
		} else {
		    notify(player,
			   tprintf("%s not found in user function table.",
				   fname));
		}
		return;
	    }

	    /* No name given, list them all. */

	    for (ufp = (UFUN *) hash_firstentry(&mudstate.ufunc_htab);
		 ufp != NULL;
		 ufp = (UFUN *) hash_nextentry(&mudstate.ufunc_htab)) {
		notify(player,
		       tprintf("%s: #%d/%s",
			       ufp->name, ufp->obj, 
			       ((ATTR *) atr_num(ufp->atr))->name));
	    }
	    return;
	}


	/* Make a local uppercase copy of the function name */

	bp = np = alloc_sbuf("add_user_func");
	safe_sb_str(fname, np, &bp);
	*bp = '\0';
	for (bp = np; *bp; bp++)
		*bp = ToLower(*bp);

	/* Verify that the function doesn't exist in the builtin table */

	if (hashfind(np, &mudstate.func_htab) != NULL) {
		notify_quiet(player,
		     "Function already defined in builtin function table.");
		free_sbuf(np);
		return;
	}
	/* Make sure the target object exists */

	if (!parse_attrib(player, target, &obj, &atr)) {
		notify_quiet(player, NOMATCH_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* Make sure the attribute exists */

	if (atr == NOTHING) {
		notify_quiet(player, "No such attribute.");
		free_sbuf(np);
		return;
	}
	/* Make sure attribute is readably by me */

	ap = atr_num(atr);
	if (!ap) {
		notify_quiet(player, "No such attribute.");
		free_sbuf(np);
		return;
	}
	atr_get_info(obj, atr, &aowner, &aflags);
	if (!See_attr(player, obj, ap, aowner, aflags)) {
		notify_quiet(player, NOPERM_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* Privileged functions require you control the obj.  */

	if ((key & FN_PRIV) && !Controls(player, obj)) {
		notify_quiet(player, NOPERM_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* See if function already exists.  If so, redefine it */

	ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab);

	if (!ufp) {
		ufp = (UFUN *) XMALLOC(sizeof(UFUN), "do_function");
		ufp->name = XSTRDUP(np, "do_function.name");
		for (bp = (char *)ufp->name; *bp; bp++)
			*bp = ToUpper(*bp);
		ufp->obj = obj;
		ufp->atr = atr;
		ufp->perms = CA_PUBLIC;
		ufp->next = NULL;
		if (!ufun_head) {
			ufun_head = ufp;
		} else {
			for (ufp2 = ufun_head; ufp2->next; ufp2 = ufp2->next) ;
			ufp2->next = ufp;
		}
		if (hashadd(np, (int *) ufp, &mudstate.ufunc_htab)) {
			notify_quiet(player, tprintf("Function %s not defined.", fname));
			XFREE(ufp->name, "do_function");
			XFREE(ufp, "do_function.2");
			free_sbuf(np);
			return;
		}
        }
	ufp->obj = obj;
	ufp->atr = atr;
	ufp->flags = key;
	free_sbuf(np);
	if (!Quiet(player))
		notify_quiet(player, tprintf("Function %s defined.", fname));
}

/* ---------------------------------------------------------------------------
 * list_functable: List available functions.
 */

void list_functable(player)
dbref player;
{
	FUN *fp;
	UFUN *ufp;
	char *buf, *bp, *cp;

	buf = alloc_lbuf("list_functable");
	bp = buf;
	for (cp = (char *)"Functions:"; *cp; cp++)
		*bp++ = *cp;
	for (fp = flist; fp->name; fp++) {
		if (check_access(player, fp->perms)) {
			*bp++ = ' ';
			for (cp = (char *)(fp->name); *cp; cp++)
				*bp++ = *cp;
		}
	}
	for (ufp = ufun_head; ufp; ufp = ufp->next) {
		if (check_access(player, ufp->perms)) {
			*bp++ = ' ';
			for (cp = (char *)(ufp->name); *cp; cp++)
				*bp++ = *cp;
		}
	}
	*bp = '\0';
	notify(player, buf);
	free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * cf_func_access: set access on functions
 */

CF_HAND(cf_func_access)
{
	FUN *fp;
	UFUN *ufp;
	char *ap;

	for (ap = str; *ap && !isspace(*ap); ap++) ;
	if (*ap)
		*ap++ = '\0';

	for (fp = flist; fp->name; fp++) {
		if (!string_compare(fp->name, str)) {
			return (cf_modify_bits(&fp->perms, ap, extra,
					       player, cmd));
		}
	}
	for (ufp = ufun_head; ufp; ufp = ufp->next) {
		if (!string_compare(ufp->name, str)) {
			return (cf_modify_bits(&ufp->perms, ap, extra,
					       player, cmd));
		}
	}
	cf_log_notfound(player, cmd, "Function", str);
	return -1;
}
