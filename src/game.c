/* game.c */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <sys/stat.h>
#include <signal.h>

#include "mudconf.h"
#include "config.h"
#include "file_c.h"
#include "command.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"
#include "attrs.h"
#include "alloc.h"
#include "slave.h"

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
#include <sys/ucontext.h>
#endif

extern void NDECL(init_attrtab);
extern void NDECL(init_cmdtab);
extern void NDECL(cf_init);
extern void NDECL(pcache_init);
extern int FDECL(cf_read, (char *fn));
extern void NDECL(init_functab);
extern void FDECL(close_sockets, (int emergency, char *message));
extern void NDECL(init_version);
extern void NDECL(init_logout_cmdtab);
extern void NDECL(init_timer);
extern void FDECL(raw_notify, (dbref, char *));
extern void FDECL(raw_notify_html, (dbref, char *));
extern void NDECL(do_second);
extern void FDECL(do_dbck, (dbref, dbref, int));
extern void NDECL(boot_slave);

void FDECL(fork_and_dump, (int));
void NDECL(dump_database);
void NDECL(pcache_sync);
void FDECL(dump_database_internal, (int));
static void NDECL(init_rlimit);
extern int FDECL(add_helpfile, (dbref, char *, int));
extern void NDECL(vattr_init);
extern void NDECL(load_restart_db);
extern int NDECL(sql_init);
extern void NDECL(sql_shutdown);

#ifndef MEMORY_BASED
extern int NDECL(dddb_optimize);
#endif

#ifndef STANDALONE
extern int FDECL(call_cron, (dbref, dbref, int, char *));
#endif

#ifdef USE_MAIL
extern void NDECL(check_mail_expiration);
#endif

#ifdef USE_COMSYS
extern void FDECL(load_comsys, (char *));
extern void FDECL(save_comsys, (char *));
extern void NDECL(update_comwho_all);
#endif

#ifdef CONCENTRATE
int conc_pid = 0;

#endif

extern pid_t slave_pid;
extern int slave_socket;

#ifdef MEMORY_BASED
extern int corrupt;
#endif

/*
 * used to allocate storage for temporary stuff, cleared before command
 * execution
 */

void do_dump(player, cause, key)
dbref player, cause;
int key;
{
        if (mudstate.dumping) {
		notify(player, "Dumping. Please try again later.");
		return;
	}
	
	notify(player, "Dumping...");
	fork_and_dump(key);
}

/* print out stuff into error file */

void NDECL(report)
{
	STARTLOG(LOG_BUGS, "BUG", "INFO")
		log_text((char *)"Command: '");
	log_text(mudstate.debug_cmd);
	log_text((char *)"'");
	ENDLOG
		if (Good_obj(mudstate.curr_player)) {
		STARTLOG(LOG_BUGS, "BUG", "INFO")
			log_text((char *)"Player: ");
		log_name_and_loc(mudstate.curr_player);
		if ((mudstate.curr_enactor != mudstate.curr_player) &&
		    Good_obj(mudstate.curr_enactor)) {
			log_text((char *)" Enactor: ");
			log_name_and_loc(mudstate.curr_enactor);
		}
		ENDLOG
	}
}

/* ----------------------------------------------------------------------
 * regexp_match: Load a regular expression match and insert it into
 * registers.
 */

int regexp_match(pattern, str, args, nargs)
char *pattern;
char *str;
char *args[];
int nargs;
{
regexp *re;
int got_match;
int i, len;

	/*
	 * Load the regexp pattern. This allocates memory which must be
	 * later freed. A free() of the regexp does free all structures
	 * under it.
	 */

	if ((re = regcomp(pattern)) == NULL) {
		/*
		 * This is a matching error. We have an error message in
		 * regexp_errbuf that we can ignore, since we're doing
		 * command-matching.
		 */
		return 0;
	}

	/* 
	 * Now we try to match the pattern. The relevant fields will
	 * automatically be filled in by this.
	 */
	got_match = regexec(re, str);
	if (!got_match) {
		free(re);
		return 0;
	}

	/*
	 * Now we fill in our args vector. Note that in regexp matching,
	 * 0 is the entire string matched, and the parenthesized strings
	 * go from 1 to 9. We DO PRESERVE THIS PARADIGM, for consistency
	 * with other languages.
	 */

	for (i = 0; i < nargs; i++) {
		args[i] = NULL;
	}

	/* Convenient: nargs and NSUBEXP are the same.
	 * We are also guaranteed that our buffer is going to be LBUF_SIZE
	 * so we can copy without fear.
	 */

	for (i = 0;
		 (i < NSUBEXP) && (re->startp[i]) && (re->endp[i]);
		 i++) {
		len = re->endp[i] - re->startp[i];
		args[i] = alloc_lbuf("regexp_match");
		strncpy(args[i], re->startp[i], len);
		args[i][len] = '\0';	/* strncpy() does not null-terminate */
	}
	
	free(re);
	return 1;
}

/* ----------------------------------------------------------------------
 * atr_match: Check attribute list for wild card matches and queue them.
 */

static int atr_match1(thing, parent, player, type, str, raw_str, check_exclude,
		      hash_insert)
dbref thing, parent, player;
char type, *str, *raw_str;
int check_exclude, hash_insert;
{
	dbref aowner;
	int match, attr, aflags, i;
	char *buff, *s, *as;
	char *args[10];
	ATTR *ap;

	/* See if we can do it.  Silently fail if we can't. */

	if (!could_doit(player, parent, A_LUSE))
		return -1;

	match = 0;
	buff = alloc_lbuf("atr_match1");
	atr_push();
	for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
		ap = atr_num(attr);

		/* Never check NOPROG attributes. */

		if (!ap || (ap->flags & AF_NOPROG))
			continue;

		/* If we aren't the bottom level check if we saw this attr
		 * before.  Also exclude it if the attribute type is PRIVATE. 
		 */

		if (check_exclude &&
		    ((ap->flags & AF_PRIVATE) ||
		     nhashfind(ap->number, &mudstate.parent_htab))) {
			continue;
		}
		atr_get_str(buff, parent, attr, &aowner, &aflags);

		/* Skip if private and on a parent */

		if (check_exclude && (aflags & AF_PRIVATE)) {
			continue;
		}
		/* If we aren't the top level remember this attr so we
		 * exclude it from now on. 
		 */

		if (hash_insert)
			nhashadd(ap->number, (int *)&attr,
				 &mudstate.parent_htab);

		/* Check for the leadin character after excluding the attrib
		 * This lets non-command attribs on the child block
		 * commands on the parent. 
		 */

		if ((buff[0] != type) || (aflags & AF_NOPROG))
			continue;

		/* decode it: search for first un escaped : */

		for (s = buff + 1; *s && (*s != ':'); s++) ;
		if (!*s)
			continue;
		*s++ = 0;
		if (((aflags & AF_REGEXP) &&
		     regexp_match(buff + 1, 
		     		((aflags & AF_NOPARSE) ? raw_str : str), 
		     		args, 10)) ||
		     wild(buff + 1, 
		     		((aflags & AF_NOPARSE) ? raw_str: str), 
		     		args, 10)) {
			match = 1;
			wait_que(thing, player, 0, NOTHING, 0, s, args, 10,
				 mudstate.global_regs);
			for (i = 0; i < 10; i++) {
				if (args[i])
					free_lbuf(args[i]);
			}
		}
	}
	free_lbuf(buff);
	atr_pop();
	return (match);
}

int atr_match(thing, player, type, str, raw_str, check_parents)
dbref thing, player;
char type, *str, *raw_str;
int check_parents;
{
	int match, lev, result, exclude, insert;
	dbref parent;

	/* If thing is halted, or it doesn't have a COMMANDS flag and we're
	 * we're doing a $-match, don't check it.
	 */
	
	if (((type == AMATCH_CMD) &&
	    !Has_Commands(thing) && mudconf.req_cmds_flag) || Halted(thing))
		return 0;  

	/* If not checking parents, just check the thing */

	match = 0;
	if (!check_parents)
		return atr_match1(thing, thing, player, type, str, raw_str, 0, 0);

	/* Check parents, ignoring halted objects */

	exclude = 0;
	insert = 1;
	nhashflush(&mudstate.parent_htab, 0);
	ITER_PARENTS(thing, parent, lev) {
		if (!Good_obj(Parent(parent)))
			insert = 0;
		result = atr_match1(thing, parent, player, type, str, raw_str,
				    exclude, insert);
		if (result > 0) {
			match = 1;
		} else if (result < 0) {
			return match;
		}
		exclude = 1;
	}

	return match;
}

/* ---------------------------------------------------------------------------
 * notify_check: notifies the object #target of the message msg, and
 * optionally notify the contents, neighbors, and location also.
 */

int check_filter(object, player, filter, msg)
dbref object, player;
int filter;
const char *msg;
{
	int aflags;
	dbref aowner;
	char *buf, *nbuf, *cp, *dp, *str, *preserve[MAX_GLOBAL_REGS];

	buf = atr_pget(object, filter, &aowner, &aflags);
	if (!*buf) {
		free_lbuf(buf);
		return (1);
	}
	save_global_regs("check_filter_save", preserve);
	nbuf = dp = alloc_lbuf("check_filter");
	str = buf;
	exec(nbuf, &dp, 0, object, player, EV_FIGNORE | EV_EVAL | EV_TOP, &str,
	     (char **)NULL, 0);
	*dp = '\0';
	dp = nbuf;
	free_lbuf(buf);
	restore_global_regs("check_filter_restore", preserve);
	
	do {
		cp = parse_to(&dp, ',', EV_STRIP);
		if (quick_wild(cp, (char *)msg)) {
			free_lbuf(nbuf);
			return (0);
		}
	} while (dp != NULL);
	free_lbuf(nbuf);
	return (1);
}

static char *add_prefix(object, player, prefix, msg, dflt)
dbref object, player;
int prefix;
const char *msg, *dflt;
{
	int aflags;
	dbref aowner;
	char *buf, *nbuf, *cp, *bp, *str, *preserve[MAX_GLOBAL_REGS];

	buf = atr_pget(object, prefix, &aowner, &aflags);
	if (!*buf) {
		cp = buf;
		safe_str((char *)dflt, buf, &cp);
	} else {
		save_global_regs("add_prefix_save", preserve);
		nbuf = bp = alloc_lbuf("add_prefix");
		str = buf;
		exec(nbuf, &bp, 0, object, player, EV_FIGNORE | EV_EVAL | EV_TOP, &str,
		     (char **)NULL, 0);
		*bp = '\0';
		free_lbuf(buf);
		restore_global_regs("add_prefix_restore", preserve);
		buf = nbuf;
		cp = &buf[strlen(buf)];
	}
	if (cp != buf) {
		safe_chr(' ', buf, &cp);
	}
	safe_str((char *)msg, buf, &cp);
	return (buf);
}

static char *dflt_from_msg(sender, sendloc)
dbref sender, sendloc;
{
	char *tp, *tbuff;

	tp = tbuff = alloc_lbuf("notify_check.fwdlist");
	safe_str((char *)"From ", tbuff, &tp);
	if (Good_obj(sendloc))
		safe_str(Name(sendloc), tbuff, &tp);
	else
		safe_str(Name(sender), tbuff, &tp);
	safe_chr(',', tbuff, &tp);
	return tbuff;
}

#ifdef PUEBLO_SUPPORT

/* Do HTML escaping, converting < to &lt;, etc.  'dest' needs to be
 * allocated & freed by the caller.
 *
 * If you're using this to append to a string, you can pass in the
 * safe_{str|chr} (char **) so we can just do the append directly,
 * saving you an alloc_lbuf()...free_lbuf().  If you want us to append
 * from the start of 'dest', just pass in a 0 for 'destp'.
 */

void html_escape(src, dest, destp)
     const char *src;
     char *dest;
     char **destp;
{
    static char new_buf[LBUF_SIZE * 5];
    const char *msg_orig;
    char *temp, *bufp;

    if (destp == 0) {
	temp = dest;
	destp = &temp;
    }

    for (*new_buf = '\0', bufp = new_buf, msg_orig = src; 
	 msg_orig && *msg_orig;
	 msg_orig++) {
	switch (*msg_orig) {
	  case '<':
	    *bufp = '\0';
	    strcat(new_buf, (char *) "&lt;");
	    bufp += 4;
	    break;
	  case '>':
	    *bufp = '\0';
	    strcat(new_buf, (char *) "&gt;"); 
	    bufp += 4;
	    break;
	  case '&':
	    *bufp = '\0';
	    strcat(new_buf, (char *) "&amp;"); 
	    bufp += 5;
	    break;
	  case '\"':
	    *bufp = '\0';
	    strcat(new_buf, (char *) "&quot;"); 
	    bufp += 6;
	    break;
	  default:
	    *bufp++ = *msg_orig;
	    break;
	}
    }
    *bufp = '\0';
    safe_str(new_buf, dest, destp);
    **destp = '\0';
}

#endif /* PUEBLO_SUPPORT */

void notify_check(target, sender, msg, key)
dbref target, sender;
int key;
const char *msg;
{
	char *msg_ns, *mp, *tbuff, *tp, *buff;
	char *args[10];
	dbref aowner, targetloc, recip, obj;
	int i, nargs, aflags, has_neighbors, pass_listen;
	int check_listens, pass_uselock, is_audible;
	FWDLIST *fp;

	/* If speaker is invalid or message is empty, just exit */

	if (!Good_obj(target) || !msg || !*msg)
		return;

	/* Enforce a recursion limit */

	mudstate.ntfy_nest_lev++;
	if (mudstate.ntfy_nest_lev >= mudconf.ntfy_nest_lim) {
		mudstate.ntfy_nest_lev--;
		return;
	}
	/* If we want NOSPOOF output, generate it.  It is only needed if 
	 * we are sending the message to the target object 
	 */

	if (key & MSG_ME) {
		mp = msg_ns = alloc_lbuf("notify_check");
		if (Nospoof(target) &&
		    (target != sender) &&
		    (target != mudstate.curr_enactor) &&
		    (target != mudstate.curr_player)) {

			/* I'd really like to use tprintf here but I can't 
			 * because the caller may have.
			 * notify(target, tprintf(...)) is quite common 
			 * in the code. 
			 */

			tbuff = alloc_sbuf("notify_check.nospoof");
			safe_chr('[', msg_ns, &mp);
			safe_str(Name(sender), msg_ns, &mp);
			*tbuff = '(';
			tbuff[1] = '#';
			ltos(&tbuff[2], sender);
			safe_str(tbuff, msg_ns, &mp);
			safe_chr(')', msg_ns, &mp);
			
			if (sender != Owner(sender)) {
				safe_chr('{', msg_ns, &mp);
				safe_str(Name(Owner(sender)), msg_ns, &mp);
				safe_chr('}', msg_ns, &mp);
			}
			if (sender != mudstate.curr_enactor) {
				strcpy(tbuff, (const char *) "<-(#");
				ltos(&tbuff[4], mudstate.curr_enactor);
				safe_str(tbuff, msg_ns, &mp);
				safe_chr(')', msg_ns, &mp);
			}
			safe_str((char *)"] ", msg_ns, &mp);
			free_sbuf(tbuff);
		}
		safe_long_str((char *)msg, msg_ns, &mp);
	} else {
		msg_ns = NULL;
	}

	/* msg contains the raw message, msg_ns contains the NOSPOOFed msg */

	check_listens = Halted(target) ? 0 : 1;
	switch (Typeof(target)) {
	case TYPE_PLAYER:
#ifndef PUEBLO_SUPPORT
		if (key & MSG_ME)
			raw_notify(target, msg_ns);
#else
		if (key & MSG_ME) {
			if (key & MSG_HTML) {
				raw_notify_html(target, msg_ns);
			} else {
				if (Html(target)) {
					char *msg_ns_escaped;

					msg_ns_escaped = alloc_lbuf("notify_check_escape");
					html_escape(msg_ns, msg_ns_escaped, 0);
					raw_notify(target, msg_ns_escaped);
					free_lbuf(msg_ns_escaped);
				} else {
					raw_notify(target, msg_ns);
				}
			}
		}	
#endif /* ! PUEBLO_SUPPORT */
		if (!mudconf.player_listen)
			check_listens = 0;
	case TYPE_THING:
	case TYPE_ROOM:

		/* If we're in a pipe, objects can receive raw_notify 
		 * if they're not a player and connected (if we didn't
		 * do this, they'd be notified twice! */
		
		if (mudstate.inpipe && (!isPlayer(target) || 
		    (isPlayer(target) && !Connected(target)))) {
			raw_notify(target, msg_ns);
		}
		
		/* Forward puppet message if it is for me */

		has_neighbors = Has_location(target);
		targetloc = where_is(target);
		is_audible = Audible(target);

		if ((key & MSG_ME) &&
		    Puppet(target) &&
		    (target != Owner(target)) &&
		    ((key & MSG_PUP_ALWAYS) ||
		     ((targetloc != Location(Owner(target))) &&
		      (targetloc != Owner(target))))) {

			tp = tbuff = alloc_lbuf("notify_check.puppet");
			safe_str(Name(target), tbuff, &tp);
			safe_str((char *)"> ", tbuff, &tp);
			safe_str(msg_ns, tbuff, &tp);
			raw_notify(Owner(target), tbuff);
			free_lbuf(tbuff);
		}
		/* Check for @Listen match if it will be useful */

		pass_listen = 0;
		nargs = 0;
		if (check_listens && (key & (MSG_ME | MSG_INV_L)) &&
		    H_Listen(target)) {
			tp = atr_get(target, A_LISTEN, &aowner, &aflags);
			if (*tp && wild(tp, (char *)msg, args, 10)) {
				for (nargs = 10;
				     nargs &&
				  (!args[nargs - 1] || !(*args[nargs - 1]));
				     nargs--) ;
				pass_listen = 1;
			}
			free_lbuf(tp);
		}
		/* If we matched the @listen or are monitoring, check the * * 
		 * USE lock 
		 */

		pass_uselock = 0;
		if ((key & MSG_ME) && check_listens &&
		    (pass_listen || Monitor(target)))
			pass_uselock = could_doit(sender, target, A_LUSE);

		/* Process AxHEAR if we pass LISTEN, USElock and it's for me */

		if ((key & MSG_ME) && pass_listen && pass_uselock) {
			if (sender != target)
				did_it(sender, target,
				       A_NULL, NULL, A_NULL, NULL,
				       A_AHEAR, args, nargs);
			else
				did_it(sender, target,
				       A_NULL, NULL, A_NULL, NULL,
				       A_AMHEAR, args, nargs);
			did_it(sender, target, A_NULL, NULL, A_NULL, NULL,
			       A_AAHEAR, args, nargs);
		}
		/* Get rid of match arguments. We don't need them anymore */

		if (pass_listen) {
			for (i = 0; i < nargs; i++)
				if (args[i] != NULL)
					free_lbuf(args[i]);
		}
		/* Process ^-listens if for me, MONITOR, and we pass USElock */

		if ((key & MSG_ME) && pass_uselock && (sender != target) &&
		    Monitor(target)) {
			(void)atr_match(target, sender,
					AMATCH_LISTEN, (char *)msg, (char *)msg, 0);
		}
		/* Deliver message to forwardlist members */

		if ((key & MSG_FWDLIST) && Audible(target) &&
		    check_filter(target, sender, A_FILTER, msg)) {
			tbuff = dflt_from_msg(sender, target);
			buff = add_prefix(target, sender, A_PREFIX,
					  msg, tbuff);
			free_lbuf(tbuff);

			fp = fwdlist_get(target);
			if (fp) {
				for (i = 0; i < fp->count; i++) {
					recip = fp->data[i];
					if (!Good_obj(recip) ||
					    (recip == target))
						continue;
					notify_check(recip, sender, buff,
						     (MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE));
				}
			}
			free_lbuf(buff);
		}
		/* Deliver message through audible exits */

		if (key & MSG_INV_EXITS) {
			DOLIST(obj, Exits(target)) {
				recip = Location(obj);
				if (Audible(obj) && ((recip != target) &&
				check_filter(obj, sender, A_FILTER, msg))) {
					buff = add_prefix(obj, target,
							  A_PREFIX, msg,
							"From a distance,");
					notify_check(recip, sender, buff,
						     MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE);
					free_lbuf(buff);
				}
			}
		}
		/* Deliver message through neighboring audible exits */

		if (has_neighbors &&
		    ((key & MSG_NBR_EXITS) ||
		     ((key & MSG_NBR_EXITS_A) && is_audible))) {

			/* If from inside, we have to add the prefix string
			 * of the container. 
			 */

			if (key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX,
						  msg, tbuff);
				free_lbuf(tbuff);
			} else {
				buff = (char *)msg;
			}

			DOLIST(obj, Exits(Location(target))) {
				recip = Location(obj);
				if (Good_obj(recip) && Audible(obj) &&
				    (recip != targetloc) &&
				    (recip != target) &&
				 check_filter(obj, sender, A_FILTER, msg)) {
					tbuff = add_prefix(obj, target,
							   A_PREFIX, buff,
							"From a distance,");
					notify_check(recip, sender, tbuff,
						     MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE);
					free_lbuf(tbuff);
				}
			}
			if (key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
		
		if (Bouncer(target))
			pass_listen = 1;
		
		/* Deliver message to contents */

		if ((key & MSG_INV) ||
		    ((key & MSG_INV_L) && pass_listen &&
		     check_filter(target, sender, A_INFILTER, msg))) {

			/* Don't prefix the message if we were given the 
			 * MSG_NOPREFIX key. 
			 */

			if (key & MSG_S_OUTSIDE) {
				buff = add_prefix(target, sender, A_INPREFIX,
						  msg, "");
			} else {
				buff = (char *)msg;
			}
			DOLIST(obj, Contents(target)) {
				if (obj != target) {
#ifdef PUEBLO_SUPPORT
					notify_check(obj, sender, buff,
					MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | (key & MSG_HTML));
#else
					notify_check(obj, sender, buff,
					MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE);
#endif /* PUEBLO_SUPPORT */
				}
			}
			if (key & MSG_S_OUTSIDE)
				free_lbuf(buff);
		}
		/* Deliver message to neighbors */

		if (has_neighbors &&
		    ((key & MSG_NBR) ||
		     ((key & MSG_NBR_A) && is_audible &&
		      check_filter(target, sender, A_FILTER, msg)))) {
			if (key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX,
						  msg, "");
				free_lbuf(tbuff);
			} else {
				buff = (char *)msg;
			}
			DOLIST(obj, Contents(targetloc)) {
				if ((obj != target) && (obj != targetloc)) {
					notify_check(obj, sender, buff,
					MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE);
				}
			}
			if (key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
		/* Deliver message to container */

		if (has_neighbors &&
		    ((key & MSG_LOC) ||
		     ((key & MSG_LOC_A) && is_audible &&
		      check_filter(target, sender, A_FILTER, msg)))) {
			if (key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX,
						  msg, tbuff);
				free_lbuf(tbuff);
			} else {
				buff = (char *)msg;
			}
			notify_check(targetloc, sender, buff,
				     MSG_ME | MSG_F_UP | MSG_S_INSIDE);
			if (key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
	}
	if (msg_ns)
		free_lbuf(msg_ns);
	mudstate.ntfy_nest_lev--;
}

void notify_except(loc, player, exception, msg)
dbref loc, player, exception;
const char *msg;
{
	dbref first;

	if (loc != exception)
		notify_check(loc, player, msg,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A));
	DOLIST(first, Contents(loc)) {
		if (first != exception) {
			notify_check(first, player, msg,
				     (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE));
		}
	}
}

void notify_except2(loc, player, exc1, exc2, msg)
dbref loc, player, exc1, exc2;
const char *msg;
{
	dbref first;

	if ((loc != exc1) && (loc != exc2))
		notify_check(loc, player, msg,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A));
	DOLIST(first, Contents(loc)) {
		if (first != exc1 && first != exc2) {
			notify_check(first, player, msg,
				     (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE));
		}
	}
}

/* ----------------------------------------------------------------------
 * Reporting of CPU information.
 */

#ifndef NO_TIMECHECKING
static void report_timecheck(player, yes_screen, yes_log, yes_clear)
    dbref player;
    int yes_screen, yes_log, yes_clear;
{
    int thing, obj_counted;
    long used_msecs, total_msecs;
    struct timeval obj_time;

    if (! (yes_log && (LOG_TIMEUSE & mudconf.log_options) != 0)) {
        yes_log = 0;
        STARTLOG(LOG_ALWAYS, "WIZ", "TIMECHECK")
            log_name(player);
            log_text((char *) " checks object time use over ");
            log_number(time(NULL) - mudstate.cpu_count_from);
            log_text((char *) " seconds\n");
        ENDLOG
   } else {
        start_log("OBJ", "CPU");
        log_name(player);
        log_text((char *) " checks object time use over ");
        log_number(time(NULL) - mudstate.cpu_count_from);
        log_text((char *) " seconds\n");
   }

    obj_counted = 0;
    total_msecs = 0;

    /* Step through the db. Care only about the ones that are nonzero.
     * And yes, we violate several rules of good programming practice
     * by failing to abstract our log calls. Oh well.
     */

    DO_WHOLE_DB(thing) {
        obj_time = Time_Used(thing);
        if (obj_time.tv_sec || obj_time.tv_usec) {
            obj_counted++;
            used_msecs = (obj_time.tv_sec * 1000) + (obj_time.tv_usec / 1000);
            total_msecs += used_msecs;
            if (yes_log)
                fprintf(stderr, "#%d\t%ld\n", thing, used_msecs);
            if (yes_screen)
                raw_notify(player, tprintf("#%d\t%ld", thing, used_msecs));
            if (yes_clear)
                obj_time.tv_usec = obj_time.tv_sec = 0;
        }
        s_Time_Used(thing, obj_time);
    }

    if (yes_screen) {
        raw_notify(player,
                   tprintf("Counted %d objects using %ld msecs over %d seconds."
,
                           obj_counted, total_msecs,
                           (int) (time(NULL) - mudstate.cpu_count_from)));
    }

    if (yes_log) {
        fprintf(stderr, "Counted %d objects using %ld msecs over %d seconds.",
                obj_counted, total_msecs,
                (int) (time(NULL) - mudstate.cpu_count_from));
        end_log();
    }

    if (yes_clear)
        mudstate.cpu_count_from = time(NULL);
}
#else
static void report_timecheck(player, yes_screen, yes_log, yes_clear)
    dbref player;
    int yes_screen, yes_log, yes_clear;
{
    raw_notify(player, "Sorry, this command has been disabled.");
}
#endif /* ! NO_TIMECHECKING */

void do_timecheck(player, cause, key)
    dbref player, cause;
    int key;
{
    int yes_screen, yes_log, yes_clear;

    yes_screen = yes_log = yes_clear = 0;

    if (key == 0) {
        /* No switches, default to printing to screen and clearing counters */
        yes_screen = 1;
        yes_clear = 1;
    } else {
        if (key & TIMECHK_RESET)
            yes_clear = 1;
        if (key & TIMECHK_SCREEN)
            yes_screen = 1;
        if (key & TIMECHK_LOG)
            yes_log = 1;
    }

    report_timecheck(player, yes_screen, yes_log, yes_clear);
}

/* ----------------------------------------------------------------------
 * Miscellaneous startup/stop routines.
 */

void do_shutdown(player, cause, key, message)
dbref player, cause;
int key;
char *message;
{
	int fd;

	if (key & SHUTDN_COREDUMP) {
		if (player != NOTHING) {
			raw_broadcast(0, "Game: Aborted by %s", Name(Owner(player)));
			STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
				log_text((char *) "Abort and coredump by ");
				log_name(player);
			ENDLOG
		}
		/* Don't bother to even shut down the network or dump. */
		/* Die. Die now. */
		abort();
	}

        if (mudstate.dumping) {
		notify(player, "Dumping. Please try again later.");
		return;
	}

	if (player != NOTHING) {
		raw_broadcast(0, "Game: Shutdown by %s", Name(Owner(player)));
		STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
			log_text((char *)"Shutdown by ");
		log_name(player);
		ENDLOG
	} else {
		raw_broadcast(0, "Game: Fatal Error: %s", message);
		STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
			log_text((char *)"Fatal error: ");
		log_text(message);
		ENDLOG
	}
	STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
		log_text((char *)"Shutdown status: ");
	log_text(message);
	ENDLOG
		fd = tf_open(mudconf.status_file, O_RDWR | O_CREAT | O_TRUNC);
	(void)write(fd, message, strlen(message));
	(void)write(fd, (char *)"\n", 1);
	tf_close(fd);

	/* Do we perform a normal or an emergency shutdown?  Normal shutdown
	 * is handled by exiting the main loop in shovechars,
	 * emergency shutdown is done here. 
	 */

	if (key & SHUTDN_PANIC) {

		/* Close down the network interface. Don't wait to
		 * try to shut down things like the external SQL
		 * db connection.
		 */

		emergency_shutdown();

		/* Close the attribute text db and dump the header db */

		pcache_sync();
		SYNC;
		CLOSE;
		STARTLOG(LOG_ALWAYS, "DMP", "PANIC")
			log_text((char *)"Panic dump: ");
		log_text(mudconf.crashdb);
		ENDLOG
			dump_database_internal(1);
		STARTLOG(LOG_ALWAYS, "DMP", "DONE")
			log_text((char *)"Panic dump complete: ");
		log_text(mudconf.crashdb);
		ENDLOG
	}
	/* Set up for normal shutdown */

	mudstate.shutdown_flag = 1;
	return;
}

void dump_database_internal(dump_type)
int dump_type;
{
	char tmpfile[256], outfn[256], prevfile[256];
	FILE *f;

	if (dump_type == 1) {
		unlink(mudconf.crashdb);
		f = tf_fopen(mudconf.crashdb, O_WRONLY | O_CREAT | O_TRUNC);
		if (f != NULL) {
			db_write(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening crash file",
				   mudconf.crashdb);
		}
#ifdef USE_MAIL
		if ((f = fopen(mudconf.mail_db, "w")) != NULL) {
		    dump_mail(f);
		    fclose(f);
		} else {
		    log_perror("DMP", "FAIL", "Opening mail file",
			       mudconf.mail_db);
		}
#endif
#ifdef USE_COMSYS
		save_comsys(mudconf.comsys_db);
#endif
		return;
	}
	
	if ((dump_type == 2) || (dump_type == 3)) {
		if (dump_type == 2) {
			f = tf_fopen(mudconf.indb, O_WRONLY | O_CREAT | O_TRUNC);
		} else {
			sprintf(tmpfile, "%s.FLAT", mudconf.outdb);
			f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
		}

		if (f != NULL) {
			/* Write a flatfile */
			db_write(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening flatfile",
				   mudconf.indb);
		}
#ifdef USE_MAIL
		if ((f = fopen(mudconf.mail_db, "w")) != NULL) {
		    dump_mail(f);
		    fclose(f);
		} else {
		    log_perror("DMP", "FAIL", "Opening mail file",
			       mudconf.mail_db);
		}
#endif
#ifdef USE_COMSYS
		save_comsys(mudconf.comsys_db);
#endif
		return;
	}
	
	if (dump_type == 4) {
		sprintf(tmpfile, "%s.KILLED", mudconf.indb);
		f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
		if (f != NULL) {
			/* Write a flatfile */
			db_write(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening killed file",
				   mudconf.indb);
		}
#ifdef USE_MAIL
		if ((f = fopen(mudconf.mail_db, "w")) != NULL) {
		    dump_mail(f);
		    fclose(f);
		} else {
		    log_perror("DMP", "FAIL", "Opening mail file",
			       mudconf.mail_db);
		}
#endif
#ifdef USE_COMSYS
		save_comsys(mudconf.comsys_db);
#endif
		return;
	}
	
	sprintf(prevfile, "%s.prev", mudconf.outdb);
#ifdef VMS
	sprintf(tmpfile, "%s.-%d-", mudconf.outdb, mudstate.epoch - 1);
	unlink(tmpfile);	/* nuke our predecessor */
	sprintf(tmpfile, "%s.-%d-", mudconf.outdb, mudstate.epoch);
#else
	sprintf(tmpfile, "%s.#%d#", mudconf.outdb, mudstate.epoch - 1);
	unlink(tmpfile);	/* nuke our predecessor */
	sprintf(tmpfile, "%s.#%d#", mudconf.outdb, mudstate.epoch);

	if (mudconf.compress_db) {
		sprintf(tmpfile, "%s.#%d#.gz", mudconf.outdb, mudstate.epoch - 1);
		unlink(tmpfile);
		sprintf(tmpfile, "%s.#%d#.gz", mudconf.outdb, mudstate.epoch);
		strcpy(outfn, mudconf.outdb);
		strcat(outfn, ".gz");
		f = tf_popen(tprintf("%s > %s", mudconf.compress, tmpfile), O_WRONLY);
		if (f) {
			db_write(f, F_TINYMUSH, OUTPUT_VERSION | OUTPUT_FLAGS);
			tf_pclose(f);
			rename(mudconf.outdb, prevfile);
			if (rename(tmpfile, outfn) < 0)
				log_perror("SAV", "FAIL",
					   "Renaming output file to DB file",
					   tmpfile);
		} else {
			log_perror("SAV", "FAIL", "Opening", tmpfile);
		}
	} else {
#endif /* VMS */
		f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
		if (f) {
			db_write(f, F_TINYMUSH, OUTPUT_VERSION | OUTPUT_FLAGS);
			tf_fclose(f);
			rename(mudconf.outdb, prevfile);
			if (rename(tmpfile, mudconf.outdb) < 0)
				log_perror("SAV", "FAIL",
				"Renaming output file to DB file", tmpfile);
		} else {
			log_perror("SAV", "FAIL", "Opening", tmpfile);
		}
#ifndef VMS
	}
#endif

#ifndef STANDALONE
#ifdef USE_MAIL
	if ((f = fopen(mudconf.mail_db, "w")) != NULL) {
	    dump_mail(f);
	    fclose(f);
	} else {
	    log_perror("SAV", "FAIL", "Opening", mudconf.mail_db);
	}
#endif
#ifdef USE_COMSYS
	save_comsys(mudconf.comsys_db);
#endif
#endif
}

void NDECL(dump_database)
{
	char *buff;

	mudstate.epoch++;
	mudstate.dumping = 1;
	buff = alloc_mbuf("dump_database");
#ifndef VMS
	sprintf(buff, "%s.#%d#", mudconf.outdb, mudstate.epoch);
#else
	sprintf(buff, "%s.-%d-", mudconf.outdb, mudstate.epoch);
#endif /* VMS */
	STARTLOG(LOG_DBSAVES, "DMP", "DUMP")
		log_text((char *)"Dumping: ");
	log_text(buff);
	ENDLOG
		pcache_sync();
	SYNC;
	dump_database_internal(0);
	STARTLOG(LOG_DBSAVES, "DMP", "DONE")
		log_text((char *)"Dump complete: ");
	log_text(buff);
	ENDLOG
	free_mbuf(buff);
	mudstate.dumping = 0;
}

void fork_and_dump(key)
int key;
{
	int child;
	char *buff;

	if (*mudconf.dump_msg)
		raw_broadcast(0, "%s", mudconf.dump_msg);
#ifdef USE_MAIL
	check_mail_expiration();
#endif
	mudstate.epoch++;
	mudstate.dumping = 1;
	buff = alloc_mbuf("fork_and_dump");
#ifndef VMS
	sprintf(buff, "%s.#%d#", mudconf.outdb, mudstate.epoch);
#else
	sprintf(buff, "%s.-%d-", mudconf.outdb, mudstate.epoch);
#endif /* VMS */
	STARTLOG(LOG_DBSAVES, "DMP", "CHKPT")
		if (!key || (key & DUMP_TEXT)) {
		log_text((char *)"SYNCing");
		if (!key || (key & DUMP_STRUCT))
			log_text((char *)" and ");
	}
	if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE)) {
		log_text((char *)"Checkpointing: ");
		log_text(buff);
	}
	ENDLOG
		free_mbuf(buff);
#ifndef MEMORY_BASED
	al_store();		/* Save cached modified attribute list */
#endif /* MEMORY_BASED */
	if (!key || (key & DUMP_TEXT))
		pcache_sync();

	if (!key || !(key & DUMP_FLATFILE)) {
		SYNC;
		OPTIMIZE;
	}
	
	if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE)) {
#ifndef VMS
		if (mudconf.fork_dump) {
			if (mudconf.fork_vfork) {
				child = vfork();
			} else {
				child = fork();
			}
		} else {
			child = 0;
		}
#else
		child = 0;
#endif /* VMS */
		if (child == 0) {
			if (key & DUMP_FLATFILE) {
				dump_database_internal(3);
			} else {
				dump_database_internal(0);
			}
#ifndef VMS
			if (mudconf.fork_dump)
				_exit(0);
#endif /* VMS */
		} else if (child < 0) {
			log_perror("DMP", "FORK", NULL, "fork()");
		}
	}
	
	if (!mudconf.fork_dump)
		mudstate.dumping = 0;
		
	if (*mudconf.postdump_msg)
		raw_broadcast(0, "%s", mudconf.postdump_msg);
}

static int NDECL(load_game)
{
	FILE *f;
	int compressed;
	char infile[256];
	struct stat statbuf;
	int db_format, db_version, db_flags;

	f = NULL;
	compressed = 0;
#ifndef VMS
	if (mudconf.compress_db) {
		strcpy(infile, mudconf.indb);
		strcat(infile, ".gz");
		if (stat(infile, &statbuf) == 0) {
			if ((f = tf_popen(tprintf(" %s < %s",
			    mudconf.uncompress, infile), O_RDONLY)) != NULL)
				compressed = 1;
		}
	}
#endif /* VMS */
	if (compressed == 0) {
		StringCopy(infile, mudconf.indb);
		if ((f = tf_fopen(mudconf.indb, O_RDONLY)) == NULL)
			return -1;
	}
	/* ok, read it in */

	STARTLOG(LOG_STARTUP, "INI", "LOAD")
		log_text((char *)"Loading: ");
	log_text(infile);
	ENDLOG
		if (db_read(f, &db_format, &db_version, &db_flags) < 0) {
		STARTLOG(LOG_ALWAYS, "INI", "FATAL")
			log_text((char *)"Error loading ");
		log_text(infile);
		ENDLOG
			return -1;
	}
#ifdef USE_COMSYS
	load_comsys(mudconf.comsys_db);
#endif
#ifdef USE_MAIL
	if ((f = fopen(mudconf.mail_db, "r")) != NULL) {
		load_mail(f);
		fclose(f);
	}
#endif
	STARTLOG(LOG_STARTUP, "INI", "LOAD")
		log_text((char *)"Load complete.");
	ENDLOG

	/* everything ok */
#ifndef VMS
		if (compressed)
		tf_pclose(f);
	else
		tf_fclose(f);
#else
		tf_fclose(f);
#endif /* VMS */

	return (0);
}

/* match a list of things, using the no_command flag */

int list_check(thing, player, type, str, raw_str, check_parent, stop_status)
dbref thing, player;
char type, *str, *raw_str;
int check_parent, *stop_status;
{
	int match;

	match = 0;
	while (thing != NOTHING) {
		if ((thing != player) &&
		    (atr_match(thing, player, type, str, raw_str,
			       check_parent) > 0)) {
		        match = 1;
			if (Stop_Match(thing)) {
			    *stop_status = 1;
			    return match;
			}
		}
		if (thing != Next(thing)) {
		        thing = Next(thing);
		} else {
		        thing = NOTHING; /* make sure we don't infinite loop */
		}
	}
	return match;
}

int Hearer(thing)
dbref thing;
{
	char *as, *buff, *s;
	dbref aowner;
	int attr, aflags;
	ATTR *ap;

	if (mudstate.inpipe && (thing == mudstate.poutobj))
		return 1;
		
	if (Connected(thing) || Puppet(thing))
		return 1;

	if (Monitor(thing))
		buff = alloc_lbuf("Hearer");
	else
		buff = NULL;
	atr_push();
	for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
		if (attr == A_LISTEN) {
			if (buff)
				free_lbuf(buff);
			atr_pop();
			return 1;
		}
		if (Monitor(thing)) {
			ap = atr_num(attr);
			if (!ap || (ap->flags & AF_NOPROG))
				continue;

			atr_get_str(buff, thing, attr, &aowner, &aflags);

			/* Make sure we can execute it */

			if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
				continue;

			/* Make sure there's a : in it */

			for (s = buff + 1; *s && (*s != ':'); s++) ;
			if (s) {
				free_lbuf(buff);
				atr_pop();
				return 1;
			}
		}
	}
	if (buff)
		free_lbuf(buff);
	atr_pop();
	return 0;
}

/* ----------------------------------------------------------------------
 * Log rotation.
 */

void do_logrotate(player, cause, key)
    dbref player, cause;
    int key;
{
    /* It would be a good idea for the initial redirection of stderr to
     * point at the same name that we've given our logfile, but this
     * isn't absolutely necessary.
     */

    mudstate.mudlognum++;
    if (!freopen((const char *) tprintf("%s.%d", mudconf.mudlogname,
					mudstate.mudlognum),
		 "w", stderr)) {
	/* Ugh. We're in trouble here. Let's try to print to stderr
	 * anyway.
	 */
	notify(player, "Log rotation failed.");
	STARTLOG(LOG_ALWAYS, "WIZ", "LOGROTATE")
	    log_name(player);
	    log_text((char *) " attempted failed log rotation");
	ENDLOG
    } else {
	notify(player, "Log rotated.");
	STARTLOG(LOG_ALWAYS, "WIZ", "LOGROTATE")
	    log_name(player);
	    log_text((char *) " rotated logfile to ");
	    log_text(mudconf.mudlogname);
	    log_text((char *) ".");
	    log_number(mudstate.mudlognum);
        ENDLOG
    }
}


/* ----------------------------------------------------------------------
 * Database and startup stuff.
 */

void do_readcache(player, cause, key)
dbref player, cause;
int key;
{
	helpindex_load(player);
	fcache_load(player);
}

static void NDECL(process_preload)
{
	dbref thing, parent, aowner;
	int aflags, lev, i;
	char *tstr;
	char tbuf[SBUF_SIZE];
	FWDLIST *fp;

	fp = (FWDLIST *) XMALLOC(sizeof(FWDLIST), "process_preload.fwdlist");
	tstr = alloc_lbuf("process_preload.string");
	i = 0;
	DO_WHOLE_DB(thing) {

		/* Ignore GOING objects */

		if (Going(thing))
			continue;

		/* Look for a FORWARDLIST attribute. Load these before
		 * doing anything else, so startup notifications work
		 * correctly.
		 */

		if (H_Fwdlist(thing)) {
			(void)atr_get_str(tstr, thing, A_FORWARDLIST,
					  &aowner, &aflags);
			if (*tstr) {
			    fp->data = NULL;
			    fwdlist_load(fp, GOD, tstr);
			    if (fp->count > 0)
				fwdlist_set(thing, fp);
			    if (fp->data) {
				XFREE(fp->data,
				      "process_preload.fwdlist_data");
			    }
#ifndef MEMORY_BASED
			    cache_reset(0);
#endif /* MEMORY_BASED */
			}
		}

		do_top(10);

		/* Look for STARTUP and DAILY attributes on parents. */

		ITER_PARENTS(thing, parent, lev) {
			if (Flags(thing) & HAS_STARTUP) {
				did_it(Owner(thing), thing,
				       A_NULL, NULL, A_NULL, NULL,
				       A_STARTUP, (char **)NULL, 0);
				/* Process queue entries as we add them */

				do_second();
				do_top(10);
#ifndef MEMORY_BASED
				cache_reset(0);
#endif /* MEMORY_BASED */
				break;
			}
			if (Flags2(thing) & HAS_DAILY) {
			    sprintf(tbuf, "0 %d * * *",
				    mudconf.events_daily_hour);
			    call_cron(thing, thing, A_DAILY, tbuf);
			}
		}
	}
	XFREE(fp, "process_preload.fwdlist");
	free_lbuf(tstr);
}

#ifndef VMS
int
#endif				/* VMS */
 main(argc, argv)
int argc;
char *argv[];
{
	int mindb;
	CMDENT *cmdp;
	int i;

	if ((argc > 2) && (!strcmp(argv[1], "-s") && (argc > 3))) {
		fprintf(stderr, "Usage: %s [-s] [config-file]\n", argv[0]);
		exit(1);
	}

#if defined(HAVE_IEEEFP_H) && defined(HAVE_SYS_UCONTEXT_H)
	/* Inhibit IEEE fp exception on overflow */

	fpsetmask(fpgetmask() & ~FP_X_OFL);
#endif

	tf_init();
#ifdef RADIX_COMPRESSION
	init_string_compress();
#endif /* RADIX_COMPRESSION */
	mindb = 0;		/* Are we creating a new db? */
	time(&mudstate.start_time);
	mudstate.restart_time = mudstate.start_time;
	time(&mudstate.cpu_count_from);
	pool_init(POOL_LBUF, LBUF_SIZE);
	pool_init(POOL_MBUF, MBUF_SIZE);
	pool_init(POOL_SBUF, SBUF_SIZE);
	pool_init(POOL_BOOL, sizeof(struct boolexp));

	pool_init(POOL_DESC, sizeof(DESC));
	pool_init(POOL_QENTRY, sizeof(BQUE));
	tcache_init();
	pcache_init();
	cf_init();
	init_rlimit();
	init_cmdtab();
	init_logout_cmdtab();
	init_flagtab();
	init_powertab();
	init_functab();
	init_attrtab();
	init_version();
	hashinit(&mudstate.player_htab, 250 * HASH_FACTOR);
	nhashinit(&mudstate.fwdlist_htab, 25 * HASH_FACTOR);
	nhashinit(&mudstate.objstack_htab, 50 * HASH_FACTOR);
	nhashinit(&mudstate.parent_htab, 5 * HASH_FACTOR);
	nhashinit(&mudstate.desc_htab, 25 * HASH_FACTOR);
	hashinit(&mudstate.vars_htab, 250 * HASH_FACTOR);
	hashinit(&mudstate.structs_htab, 50 * HASH_FACTOR);
	hashinit(&mudstate.cdefs_htab, 100 * HASH_FACTOR);
	hashinit(&mudstate.instance_htab, 100 * HASH_FACTOR);
	hashinit(&mudstate.instdata_htab, 250 * HASH_FACTOR);
#ifdef USE_MAIL
	nhashinit(&mudstate.mail_htab, 50 * HASH_FACTOR);
#endif

	add_helpfile(GOD, (char *) "help text/help", 1);
	add_helpfile(GOD, (char *) "wizhelp text/wizhelp", 1);
	add_helpfile(GOD, (char *) "news text/news", 0);
	cmdp = (CMDENT *) hashfind((char *) "wizhelp", &mudstate.command_htab);
	if (cmdp)
	    cmdp->perms |= CA_WIZARD;

	vattr_init();
	
	if (argc > 1 && !strcmp(argv[1], "-s")) {
		mindb = 1;
		if (argc == 3)
			cf_read(argv[2]);
		else
			cf_read((char *)CONF_FILE);
	} else if (argc == 2) {
		cf_read(argv[1]);
	} else {
		cf_read((char *)CONF_FILE);
	}

	strncpy(mudconf.exec_path, argv[0], PBUF_SIZE - 1);
	mudconf.exec_path[PBUF_SIZE - 1] = '\0';
	
	fcache_init();
	helpindex_init();

#ifndef MEMORY_BASED
	if (mindb)
		unlink(mudconf.gdbm);
	if (init_gdbm_db(mudconf.gdbm) < 0) {
		STARTLOG(LOG_ALWAYS, "INI", "LOAD")
			log_text((char *)"Couldn't load text database: ");
		log_text(mudconf.gdbm);
		ENDLOG
			exit(2);
	}
#else
	db_free();
#endif /*
        * MEMORY_BASED 
        */

	mudstate.record_players = 0;
	
	mudstate.loading_db = 1;
	if (mindb)
		db_make_minimal();
	else if (load_game() < 0) {
		STARTLOG(LOG_ALWAYS, "INI", "LOAD")
			log_text((char *)"Couldn't load: ");
		log_text(mudconf.indb);
		ENDLOG
			exit(2);
	}
	mudstate.loading_db = 0;

	srandom(getpid());
	set_signals();

	/* Do a consistency check and set up the freelist */

	do_dbck(NOTHING, NOTHING, 0);

	/* Reset all the hash stats */

	hashreset(&mudstate.command_htab);
	hashreset(&mudstate.logout_cmd_htab);
	hashreset(&mudstate.func_htab);
	hashreset(&mudstate.ufunc_htab);
	hashreset(&mudstate.powers_htab);
	hashreset(&mudstate.flags_htab);
	hashreset(&mudstate.attr_name_htab);
	hashreset(&mudstate.vattr_name_htab);
	hashreset(&mudstate.player_htab);
	nhashreset(&mudstate.desc_htab);
	nhashreset(&mudstate.fwdlist_htab);
	nhashreset(&mudstate.objstack_htab);
	nhashreset(&mudstate.parent_htab);
	hashreset(&mudstate.vars_htab);
	hashreset(&mudstate.structs_htab);
	hashreset(&mudstate.cdefs_htab);
	hashreset(&mudstate.instance_htab);
	hashreset(&mudstate.instdata_htab);
#ifdef USE_COMSYS
	hashreset(&mudstate.comsys_htab);
	hashreset(&mudstate.calias_htab);
	nhashreset(&mudstate.comlist_htab);
#endif
#ifdef USE_MAIL
	nhashreset(&mudstate.mail_htab);
#endif

	for (i = 0; i < mudstate.helpfiles; i++)
	    hashreset(&mudstate.hfile_hashes[i]);

	for (mindb = 0; mindb < MAX_GLOBAL_REGS; mindb++)
		mudstate.global_regs[mindb] = alloc_lbuf("main.global_reg");
	mudstate.now = time(NULL);

	load_restart_db();

	if (!mudstate.restarting) {
	    /*
	     * CAUTION:
	     * We do this here rather than up at the top of this function
	     * because we need to know if we're restarting. If we are,
	     * our previous process closed stdin and stdout at inception,
	     * and therefore we don't need to do so.
	     * More importantly, on a restart, the file descriptors normally
	     * allocated to stdin and stdout may have been used for player
	     * socket descriptors. Thus, closing them like streams is
	     * really, really bad.
	     */
	    fclose(stdin);
	    fclose(stdout);
	}

	/* We have to do an update, even though we're starting up, because
	 * there may be players connected from a restart, as well as objects.
	 */
#ifdef USE_COMSYS
	update_comwho_all();
#endif

	sql_init();		/* Make a connection to external SQL db */

	/* You must do your startups AFTER you load your restart database,
	 * or softcode that depends on knowing who is connected and so forth
	 * will be hosed.
	 */
	process_preload();
	STARTLOG(LOG_STARTUP, "INI", "LOAD")
	    log_text((char *) "Startup processing complete.");
	ENDLOG

	boot_slave();

	if (mudstate.restarting) {
	    raw_broadcast(0, "Game: Restart finished.");
	}

#ifdef CONCENTRATE
	if (!mudstate.restarting) {
		/*
		 * Start up the port concentrator. 
		 */

		conc_pid = fork();
		if (conc_pid < 0) {
			perror("fork");
			exit(-1);
		}
		if (conc_pid == 0) {
			char mudp[32], inetp[32];

			/*
			 * Add port argument to concentrator 
			 */
			sprintf(mudp, "%d", mudconf.port);
			sprintf(inetp, "%d", mudconf.conc_port);
			execl("./bin/conc", "concentrator", inetp, mudp, "1", 0);
		}
		STARTLOG(LOG_ALWAYS, "CNC", "STRT")
			log_text("Concentrating ports... ");
		log_text(tprintf("Main: %d Conc: %d", mudconf.port, mudconf.conc_port));
		ENDLOG
	}
#endif

#ifdef MCHECK
	mtrace();
#endif

	/* go do it */

	init_timer();
	shovechars(mudconf.port);

#ifdef MCHECK
	muntrace();
#endif

	sql_shutdown();		/* Terminate external SQL db connection */
	close_sockets(0, (char *)"Going down - Bye");
	dump_database();
	CLOSE;

	if (slave_socket != -1) {
		kill(slave_pid, SIGKILL);
	}
#ifdef CONCENTRATE
	kill(conc_pid, SIGKILL);
#endif

	exit(0);
}

static void NDECL(init_rlimit)
{
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
	struct rlimit *rlp;

	rlp = (struct rlimit *) XMALLOC(sizeof(struct rlimit), "rlimit");

	if (getrlimit(RLIMIT_NOFILE, rlp)) {
		log_perror("RLM", "FAIL", NULL, "getrlimit()");
		free_lbuf(rlp);
		return;
	}
	rlp->rlim_cur = rlp->rlim_max;
	if (setrlimit(RLIMIT_NOFILE, rlp))
		log_perror("RLM", "FAIL", NULL, "setrlimit()");
	XFREE(rlp, "rlimit");
#else
#if defined(_SEQUENT_) && defined(NUMFDS_LIMIT)
	setdtablesize(NUMFDS_LIMIT);
#endif /* Sequent and unlimiting #define'd */
#endif /* HAVE_SETRLIMIT */
}
