/* funext.c - Functions that rely on external call-outs */
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

#include "functions.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "db_sql.h"	/* required by code */

extern void FDECL(make_ulist, (dbref, char *, char **));
extern void FDECL(make_portlist, (dbref, dbref, char *, char **));
extern int FDECL(fetch_idle, (dbref));
extern int FDECL(fetch_connect, (dbref));
extern char * FDECL(get_doing, (dbref));
extern dbref FDECL(get_programmer, (dbref));
extern void FDECL(cf_display, (dbref, char *, char *, char **));
extern INLINE int FDECL(safe_chr_real_fn, (char, char *, char **, int));

#ifdef USE_MAIL
extern INLINE char *FDECL(get_mail_message, (int));
extern void FDECL(count_mail, (dbref, int, int *, int *, int *));
#endif

/* ---------------------------------------------------------------------------
 * config: Display a MUSH config parameter.
 */

FUNCTION(fun_config)
{
    cf_display(player, fargs[0], buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_lwho: Return list of connected users.
 */

FUNCTION(fun_lwho)
{
	make_ulist(player, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_ports: Returns a list of ports for a user.
 */

FUNCTION(fun_ports)
{
	dbref target;

	if (!Wizard(player)) {
		return;
	}
	target = lookup_player(player, fargs[0], 1);
	if (!Good_obj(target) || !Connected(target)) {
		return;
	}
	make_portlist(player, target, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_doing: Returns a user's doing.
 */

FUNCTION(fun_doing)
{
    dbref target;
    char *str;

    target = lookup_player(player, fargs[0], 1);
    if (!Good_obj(target) || !Connected(target))
	return;

    if ((str = get_doing(target)) != NULL)
	safe_str(get_doing(target), buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_idle, fun_conn: return seconds idle or connected.
 */

FUNCTION(fun_idle)
{
	dbref target;

	target = lookup_player(player, fargs[0], 1);
	if (Good_obj(target) && Hidden(target) && !See_Hidden(player))
		target = NOTHING;
	safe_ltos(buff, bufc, fetch_idle(target));
}

FUNCTION(fun_conn)
{
	dbref target;

	target = lookup_player(player, fargs[0], 1);
	if (Good_obj(target) && Hidden(target) && !See_Hidden(player))
		target = NOTHING;
	safe_ltos(buff, bufc, fetch_connect(target));
}

/* ---------------------------------------------------------------------------
 * fun_programmer: Returns the dbref or #1- of an object in a @program.
 */

FUNCTION(fun_programmer)
{
    dbref target;

    target = lookup_player(player, fargs[0], 1);
    if (!Good_obj(target) || !Connected(target) ||
	!Examinable(player, target)) {
	safe_nothing(buff, bufc);
	return;
    }
    safe_dbref(buff, bufc, get_programmer(target));
}

/*---------------------------------------------------------------------------
 * Mail functions borrowed from DarkZone
 */

#ifdef USE_MAIL

FUNCTION(fun_mail)
{
	/* This function can take one of three formats: 1.  mail(num)  -->
	 * returns message <num> for privs. 2.  mail(player)  -->
	 * returns number of messages for <player>. 3.
	 * mail(player, num)  -->  returns message <num> for
	 * <player>. 
	 *
	 * It can now take one more format: 4.  mail() --> returns number of
	 * messages for executor 
	 */

	struct mail *mp;
	dbref playerask;
	int num, rc, uc, cc;
#ifdef RADIX_COMPRESSION
	char *msgbuff;
#endif

	if (!fn_range_check("MAIL", nfargs, 0, 2, buff, bufc))
		return;
	if ((nfargs == 0) || !fargs[0] || !fargs[0][0]) {
		count_mail(player, 0, &rc, &uc, &cc);
		safe_ltos(buff, bufc, rc + uc);
		return;
	}
	if (nfargs == 1) {
		if (!is_number(fargs[0])) {
			/* handle the case of wanting to count the number of
			 * messages 
			 */
			if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
				safe_str("#-1 NO SUCH PLAYER", buff, bufc);
				return;
			} else if ((player != playerask) && !Wizard(player)) {
				safe_noperm(buff, bufc);
				return;
			} else {
				count_mail(playerask, 0, &rc, &uc, &cc);
				safe_tprintf_str(buff, bufc, "%d %d %d", rc, uc, cc);
				return;
			}
		} else {
			playerask = player;
			num = atoi(fargs[0]);
		}
	} else {
		if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
			safe_str("#-1 NO SUCH PLAYER", buff, bufc);
			return;
		} else if ((player != playerask) && !God(player)) {
			safe_noperm(buff, bufc);
			return;
		}
		num = atoi(fargs[1]);
	}

	if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER)) {
		safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
		return;
	}
	mp = mail_fetch(playerask, num);
	if (mp != NULL) {
#ifdef RADIX_COMPRESSION
		msgbuff = alloc_lbuf("fun_mail");
		string_decompress(get_mail_message(mp->number), msgbuff);
		safe_str(msgbuff, buff, bufc);
		free_lbuf(msgbuff);
#else
		safe_str(get_mail_message(mp->number), buff, bufc);
#endif
		return;
	}
	/* ran off the end of the list without finding anything */
	safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
}

FUNCTION(fun_mailfrom)
{
	/* This function can take these formats: 1) mailfrom(<num>) 2)
	 * mailfrom(<player>,<num>) It returns the dbref of the player the
	 * mail is from 
	 */
	struct mail *mp;
	dbref playerask;
	int num;

	if (!fn_range_check("MAILFROM", nfargs, 1, 2, buff, bufc))
		return;
	if (nfargs == 1) {
		playerask = player;
		num = atoi(fargs[0]);
	} else {
		if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
			safe_str("#-1 NO SUCH PLAYER", buff, bufc);
			return;
		} else if ((player != playerask) && !Wizard(player)) {
			safe_noperm(buff, bufc);
			return;
		}
		num = atoi(fargs[1]);
	}

	if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER)) {
		safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
		return;
	}
	mp = mail_fetch(playerask, num);
	if (mp != NULL) {
		safe_dbref(buff, bufc, mp->from);
		return;
	}
	/* ran off the end of the list without finding anything */
	safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
}
#endif /* USE_MAIL */

/*---------------------------------------------------------------------------
 * SQL stuff.
 */

FUNCTION(fun_sql)
{
    char row_delim, field_delim;

    /* Special -- the last two arguments are output delimiters */

    if (!fn_range_check("SQL", nfargs, 1, 3, buff, bufc))
	return;
    if (!delim_check(fargs, nfargs, 2, &row_delim, buff, bufc, 0,
		     player, cause, cargs, ncargs, 1))
	return;
    if (nfargs < 3)
	field_delim = row_delim;
    else if (!delim_check(fargs, nfargs, 3, &field_delim, buff, bufc, 0,
			  player, cause, cargs, ncargs, 1))
	return;

    sql_query(player, fargs[0], buff, bufc, row_delim, field_delim);
}

/*---------------------------------------------------------------------------
 * Pueblo HTML-related functions.
 */

#ifdef PUEBLO_SUPPORT

FUNCTION(fun_html_escape)
{
    html_escape(fargs[0], buff, bufc);
}

FUNCTION(fun_html_unescape)
{
    const char *msg_orig;
    int ret = 0;

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++) {
	switch (*msg_orig) {
	  case '&':
	    if (!strncmp(msg_orig, "&quot;", 6)) {
		ret = safe_chr_fn('\"', buff, bufc);
		msg_orig += 5;
	    } else if (!strncmp(msg_orig, "&lt;", 4)) {
		ret = safe_chr_fn('<', buff, bufc);
		msg_orig += 3;
	    } else if (!strncmp(msg_orig, "&gt;", 4)) {
		ret = safe_chr_fn('>', buff, bufc);
		msg_orig += 3;
	    } else if (!strncmp(msg_orig, "&amp;", 5)) {
		ret = safe_chr_fn('&', buff, bufc);
		msg_orig += 4;
	    }
	    break;
	  default:
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	    break;
	}
    }
}

FUNCTION(fun_url_escape)
{
    /* These are the characters which are converted to %<hex> */
    char *escaped_chars = "<>#%{}|\\^~[]';/?:@=&\"+";
    const char *msg_orig;
    int ret = 0;
    char tbuf[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++) {
	if (strchr(escaped_chars, *msg_orig)) {
	    sprintf(tbuf, "%%%2x", *msg_orig);
	    ret = safe_str(tbuf, buff, bufc);
	} else if (*msg_orig == ' ') {
	    ret = safe_chr_fn('+', buff, bufc);
	} else{
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	}
    }
}

FUNCTION(fun_url_unescape)
{
    const char *msg_orig;
    int ret = 0;
    unsigned int tempchar;
    char tempstr[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret;) {
	switch (*msg_orig) {
	  case '+':
	    ret = safe_chr_fn(' ', buff, bufc);
	    msg_orig++;
	    break;
	  case '%':
	    strncpy(tempstr, msg_orig+1, 2);
	    tempstr[2] = '\0';
	    if ((sscanf(tempstr, "%x", &tempchar) == 1) &&
		(tempchar > 0x1F) && (tempchar < 0x7F)) {
		ret = safe_chr_fn(tempchar, buff, bufc);
	    }
	    if (*msg_orig)
		msg_orig++;	/* Skip the '%' */
	    if (*msg_orig) 	/* Skip the 1st hex character. */
		msg_orig++;
	    if (*msg_orig)	/* Skip the 2nd hex character. */
		msg_orig++;
	    break;
	  default:
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	    msg_orig++;
	    break;
	}
    }
    return;
}
#endif /* PUEBLO_SUPPORT */
