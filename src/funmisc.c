/* funmisc.c - misc functions */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "functions.h"

extern NAMETAB indiv_attraccess_nametab[];

extern void FDECL(do_pemit_list, (dbref, char *, const char *, int));

/* ---------------------------------------------------------------------------
 * fun_switch: Return value based on pattern matching (ala @switch/first)
 * fun_switchall: Similar, but ala @switch/all
 * fun_case: Like switch(), but a straight exact match instead of wildcard.
 * NOTE: These functions expect that their arguments have not been evaluated.
 */

FUNCTION(fun_switchall)
{
    int i, got_one;
    char *mbuff, *tbuff, *bp, *str, *save_token;

    /* If we don't have at least 2 args, return nothing */

    if (nfargs < 2) {
	return;
    }

    /* Evaluate the target in fargs[0] */

    mbuff = bp = alloc_lbuf("fun_switchall");
    str = fargs[0];
    exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	 &str, cargs, ncargs);
    *bp = '\0';

    /* Loop through the patterns looking for a match */

    mudstate.in_switch++;
    save_token = mudstate.switch_token;

    got_one = 0;
    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
	tbuff = bp = alloc_lbuf("fun_switchall.2");
	str = fargs[i];
	exec(tbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, cargs, ncargs);
	*bp = '\0';
	if (quick_wild(tbuff, mbuff)) {
	    got_one = 1;
	    free_lbuf(tbuff);
	    mudstate.switch_token = mbuff;
	    str = fargs[i+1];
	    exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		 &str, cargs, ncargs);
	} else {
	    free_lbuf(tbuff);
	}
    }
    
    /* If we didn't match, return the default if there is one */
    
    if (!got_one && (i < nfargs) && fargs[i]) {
	mudstate.switch_token = mbuff;
	str = fargs[i];
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, cargs, ncargs);
    }

    free_lbuf(mbuff);
    mudstate.in_switch--;
    mudstate.switch_token = save_token;
}

FUNCTION(fun_switch)
{
	int i;
	char *mbuff, *tbuff, *bp, *str, *save_token;

	/* If we don't have at least 2 args, return nothing */

	if (nfargs < 2) {
		return;
	}
	/* Evaluate the target in fargs[0] */

	mbuff = bp = alloc_lbuf("fun_switch");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, cargs, ncargs);
	*bp = '\0';

	/* Loop through the patterns looking for a match */

	mudstate.in_switch++;
	save_token = mudstate.switch_token;

	for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
		tbuff = bp = alloc_lbuf("fun_switch.2");
		str = fargs[i];
		exec(tbuff, &bp, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL,
		     &str, cargs, ncargs);
		*bp = '\0';
		if (quick_wild(tbuff, mbuff)) {
			free_lbuf(tbuff);
			mudstate.switch_token = mbuff;
			str = fargs[i+1];
			exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			     &str, cargs, ncargs);
			free_lbuf(mbuff);
			mudstate.in_switch--;
			mudstate.switch_token = save_token;
			return;
		}
		free_lbuf(tbuff);
	}

	/* Nope, return the default if there is one */

	if ((i < nfargs) && fargs[i]) {
	        mudstate.switch_token = mbuff;
		str = fargs[i];
		exec(buff, bufc, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL,
		     &str, cargs, ncargs);
	}
	free_lbuf(mbuff);
	mudstate.in_switch--;
	mudstate.switch_token = save_token;
}

FUNCTION(fun_case)
{
	int i;
	char *mbuff, *bp, *str;

	/* If we don't have at least 2 args, return nothing */

	if (nfargs < 2) {
		return;
	}
	/* Evaluate the target in fargs[0] */

	mbuff = bp = alloc_lbuf("fun_case");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, cargs, ncargs);
	*bp = '\0';

	/* Loop through the patterns looking for a case-insensitive match */

	for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
	    if (!string_compare(fargs[i], mbuff)) {
		str = fargs[i + 1];
		exec(buff, bufc, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
		free_lbuf(mbuff);
		return;
	    }
	}
	free_lbuf(mbuff);

	/* Nope, return the default if there is one */

	if ((i < nfargs) && fargs[i]) {
	    str = fargs[i];
	    exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		 &str, cargs, ncargs);
	}
	return;
}

FUNCTION(fun_ifelse)
{
	/* This function now assumes that its arguments have not been
	   evaluated. */
	
	char *str, *mbuff, *bp;
	
	mbuff = bp = alloc_lbuf("fun_ifelse");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		&str, cargs, ncargs);
	*bp = '\0';
	
	if (!mbuff || !*mbuff || !xlate(mbuff)) {
		str = fargs[2];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	} else {
		str = fargs[1];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	}
	free_lbuf(mbuff);
}

FUNCTION(fun_nonzero)
{
	/* MUX-style ifelse -- rather than bool check, check if the
	* string is non-null/non-zero.
	*/
	
	char *str, *mbuff, *bp;
	
	mbuff = bp = alloc_lbuf("fun_nonzero");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		&str, cargs, ncargs);
	*bp = '\0';
	
	if (!mbuff || !*mbuff || ((atoi(mbuff) == 0) && is_number(mbuff))) {
		str = fargs[2];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	} else {
		str = fargs[1];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	}
	free_lbuf(mbuff);
}

/* ---------------------------------------------------------------------------
 * fun_rand: Return a random number from 0 to arg1-1
 */

FUNCTION(fun_rand)
{
	int num;

	num = atoi(fargs[0]);
	if (num < 1) {
		safe_chr('0', buff, bufc);
	} else
		safe_tprintf_str(buff, bufc, "%ld",
				 (long) (makerandom() * num));
}

/* ---------------------------------------------------------------------------
 * die(<number of dice>,<sides>): Roll XdY dice.
 * lrand(<range bottom>,<range top>,<times>[,<delim>]): Generate random list.
 */

FUNCTION(fun_die)
{
	int n, die, count;
	int total = 0;

	if (!fargs[0] || !fargs[1]) {
	    safe_chr('0', buff, bufc);
	    return;
	}

	n = atoi(fargs[0]);
	die = atoi(fargs[1]);

	if ((n == 0) || (die <= 0)) {
	    safe_chr('0', buff, bufc);
	    return;
	}

	if ((n < 1) || (n > 100)) {
		safe_str("#-1 NUMBER OUT OF RANGE", buff, bufc);
		return;
	}
	for (count = 0; count < n; count++)
		total += (int) (makerandom() * die) + 1;

	safe_ltos(buff, bufc, total);
}


FUNCTION(fun_lrand)
{
    char sep;
    int n_times, r_bot, r_top, i;
    double n_range;
    unsigned int tmp;
    char *bb_p;

    /* Special: the delim is really an output delim. */

    if (!fn_range_check("LRAND", nfargs, 3, 4, buff, bufc))
	return;
    if (!delim_check(fargs, nfargs, 4, &sep, buff, bufc, 0,
		     player, cause, cargs, ncargs, 1))
	return;

    /* If we're generating no numbers, since this is a list function,
     * we return empty, rather than returning 0.
     */

    n_times = atoi(fargs[2]);
    if (n_times < 1) {
	return;
    }
    if (n_times > LBUF_SIZE) {
	n_times = LBUF_SIZE;
    }
    r_bot = atoi(fargs[0]);
    r_top = atoi(fargs[1]);

    if (r_top < r_bot) {

	/* This is an error condition. Just return an empty list. We
	 * obviously can't return a random number between X and Y if
	 * Y is less than X.
	 */

	return;

    } else if (r_bot == r_top) {

	/* Just generate a list of n repetitions. */

	bb_p = *bufc;
	for (i = 0; i < n_times; i++) {
	    if (*bufc != bb_p) {
		print_sep(sep, buff, bufc);
	    }
	    safe_ltos(buff, bufc, r_bot);
	}
	return;
    }

    /* We've hit this point, we have a range. Generate a list. */

    n_range = (double) r_top - r_bot + 1;
    bb_p = *bufc;
    for (i = 0; i < n_times; i++) {
	if (*bufc != bb_p) {
	    print_sep(sep, buff, bufc);
	}
	tmp = (unsigned int) (makerandom() * n_range);
	safe_ltos(buff, bufc, r_bot + tmp);
    }
}

/* ---------------------------------------------------------------------------
 * fun_lnum: Return a list of numbers.
 */

FUNCTION(fun_lnum)
{
    char tbuf[12], sep;
    int bot, top, over, i;
    char *bb_p;

    if (nfargs == 0) {
	return;
    }

    /* lnum() is special, since its single delimiter is really an output
     * delimiter.
     */
    if (!fn_range_check("LNUM", nfargs, 1, 3, buff, bufc))
	return;
    if (!delim_check(fargs, nfargs, 3, &sep, buff, bufc, 0,
		     player, cause, cargs, ncargs, 1))
	return;

    if (nfargs >= 2) {
	bot = atoi(fargs[0]);
	top = atoi(fargs[1]);
    } else {
	bot = 0;
	top = atoi(fargs[0]);
	if (top-- < 1)		/* still want to generate if arg is 1 */
	    return;
    }

    over = 0;
    bb_p = *bufc;

    if (top == bot) {
	safe_ltos(buff, bufc, bot);
	return;
    } else if (top > bot) {
	for (i = bot; (i <= top) && !over; i++) {
	    if (*bufc != bb_p) {
		print_sep(sep, buff, bufc);
	    }
	    ltos(tbuf, i);
	    over = safe_str(tbuf, buff, bufc);
	}
    } else {
	for (i = bot; (i >= top) && !over; i--) {
	    if (*bufc != bb_p) {
		print_sep(sep, buff, bufc);
	    }
	    ltos(tbuf, i);
	    over = safe_str(tbuf, buff, bufc);
	}
    }
}

/* ---------------------------------------------------------------------------
 * fun_time: Returns nicely-formatted time.
 */

FUNCTION(fun_time)
{
	char *temp;

	temp = (char *)ctime(&mudstate.now);
	temp[strlen(temp) - 1] = '\0';
	safe_str(temp, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_time: Seconds since 0:00 1/1/70
 */

FUNCTION(fun_secs)
{
	safe_ltos(buff, bufc, mudstate.now);
}

/* ---------------------------------------------------------------------------
 * fun_convsecs: converts seconds to time string, based off 0:00 1/1/70
 */

FUNCTION(fun_convsecs)
{
	char *temp;
	time_t tt;

	tt = atol(fargs[0]);
	temp = (char *)ctime(&tt);
	temp[strlen(temp) - 1] = '\0';
	safe_str(temp, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_convtime: converts time string to seconds, based off 0:00 1/1/70
 *    additional auxiliary function and table used to parse time string,
 *    since no ANSI standard function are available to do this.
 */

static const char *monthtab[] =
{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const char daystab[] =
{31, 29, 31, 30, 31, 30,
 31, 31, 30, 31, 30, 31};

/* converts time string to a struct tm. Returns 1 on success, 0 on fail.
 * Time string format is always 24 characters long, in format
 * Ddd Mmm DD HH:MM:SS YYYY
 */

#define	get_substr(buf, p) { \
	p = (char *)index(buf, ' '); \
	if (p) { \
		*p++ = '\0'; \
		while (*p == ' ') p++; \
	} \
}

int do_convtime(str, ttm)
char *str;
struct tm *ttm;
{
	char *buf, *p, *q;
	int i;

	if (!str || !ttm)
		return 0;
	while (*str == ' ')
		str++;
	buf = p = alloc_sbuf("do_convtime");	/* make a temp copy of arg */
	safe_sb_str(str, buf, &p);
	*p = '\0';

	get_substr(buf, p);	/* day-of-week or month */
	if (!p || strlen(buf) != 3) {
		free_sbuf(buf);
		return 0;
	}
	for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++) ;
	if (i == 12) {
		get_substr(p, q);	/* month */
		if (!q || strlen(p) != 3) {
			free_sbuf(buf);
			return 0;
		}
		for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++) ;
		if (i == 12) {
			free_sbuf(buf);
			return 0;
		}
		p = q;
	}
	ttm->tm_mon = i;

	get_substr(p, q);	/* day of month */
	if (!q || (ttm->tm_mday = atoi(p)) < 1 || ttm->tm_mday > daystab[i]) {
		free_sbuf(buf);
		return 0;
	}
	p = (char *)index(q, ':');	/* hours */
	if (!p) {
		free_sbuf(buf);
		return 0;
	}
	*p++ = '\0';
	if ((ttm->tm_hour = atoi(q)) > 23 || ttm->tm_hour < 0) {
		free_sbuf(buf);
		return 0;
	}
	if (ttm->tm_hour == 0) {
		while (isspace(*q))
			q++;
		if (*q != '0') {
			free_sbuf(buf);
			return 0;
		}
	}
	q = (char *)index(p, ':');	/* minutes */
	if (!q) {
		free_sbuf(buf);
		return 0;
	}
	*q++ = '\0';
	if ((ttm->tm_min = atoi(p)) > 59 || ttm->tm_min < 0) {
		free_sbuf(buf);
		return 0;
	}
	if (ttm->tm_min == 0) {
		while (isspace(*p))
			p++;
		if (*p != '0') {
			free_sbuf(buf);
			return 0;
		}
	}
	get_substr(q, p);	/* seconds */
	if (!p || (ttm->tm_sec = atoi(q)) > 59 || ttm->tm_sec < 0) {
		free_sbuf(buf);
		return 0;
	}
	if (ttm->tm_sec == 0) {
		while (isspace(*q))
			q++;
		if (*q != '0') {
			free_sbuf(buf);
			return 0;
		}
	}
	get_substr(p, q);	/* year */
	if ((ttm->tm_year = atoi(p)) == 0) {
		while (isspace(*p))
			p++;
		if (*p != '0') {
			free_sbuf(buf);
			return 0;
		}
	}
	free_sbuf(buf);
	if (ttm->tm_year > 100)
		ttm->tm_year -= 1900;
	if (ttm->tm_year < 0) {
		return 0;
	}

	/* We don't whether or not it's daylight savings time. */
	ttm->tm_isdst = -1;
        
#define LEAPYEAR_1900(yr) ((yr)%400==100||((yr)%100!=0&&(yr)%4==0))
	return (ttm->tm_mday != 29 || i != 1 || LEAPYEAR_1900(ttm->tm_year));
#undef LEAPYEAR_1900
}

FUNCTION(fun_convtime)
{
	struct tm *ttm;

	ttm = localtime(&mudstate.now);
	if (do_convtime(fargs[0], ttm))
		safe_ltos(buff, bufc, timelocal(ttm));
	else
		safe_known_str("-1", 2, buff, bufc);
}


/* ---------------------------------------------------------------------------
 * fun_starttime: What time did this system last reboot?
 */

FUNCTION(fun_starttime)
{
	char *temp;

	temp = (char *)ctime(&mudstate.start_time);
	temp[strlen(temp) - 1] = '\0';
	safe_str(temp, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_restarts: How many times have we restarted?
 */

FUNCTION(fun_restarts)
{
    safe_ltos(buff, bufc, mudstate.reboot_nums);
}

/* ---------------------------------------------------------------------------
 * fun_restarttime: When did we last restart?
 */

FUNCTION(fun_restarttime)
{
	char *temp;

	temp = (char *)ctime(&mudstate.restart_time);
	temp[strlen(temp) - 1] = '\0';
	safe_str(temp, buff, bufc);
}

FUNCTION(fun_version)
{
	safe_str(mudstate.version, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_mudname: Return the name of the mud.
 */

FUNCTION(fun_mudname)
{
	safe_str(mudconf.mud_name, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_s: Force substitution to occur.
 * fun_subeval: Like s(), but don't do function evaluations.
 */

FUNCTION(fun_s)
{
	char *str;

	str = fargs[0];
	exec(buff, bufc, 0, player, cause, EV_FIGNORE | EV_EVAL, &str,
	     cargs, ncargs);
}

FUNCTION(fun_subeval)
{
	char *str;
	
	str = fargs[0];
	exec(buff, bufc, 0, player, cause,
	     EV_NO_LOCATION|EV_NOFCHECK|EV_FIGNORE|EV_NO_COMPRESS,
	     &str, (char **)NULL, 0);
}

/*------------------------------------------------------------------------
 * Side-effect functions.
 */

static int check_command(player, name, buff, bufc)
dbref player;
char *name, *buff, **bufc;
{
    CMDENT *cmdp;

    if ((cmdp = (CMDENT *) hashfind(name, &mudstate.command_htab))) {

	/* Note that these permission checks are NOT identical to the
	 * ones in process_cmdent(). In particular, side-effects are NOT
	 * subject to the CA_GBL_INTERP flag. This is a design decision
	 * based on the concept that these are functions and not commands,
	 * even though they behave like commands in many respects. This
	 * is also the same reason why side-effects don't trigger hooks.
	 */

	if (Invalid_Objtype(player) || !check_access(player, cmdp->perms) ||
	    (!Builder(player) && Protect(CA_GBL_BUILD) &&
	     !(mudconf.control_flags & CF_BUILD))) {

	    safe_noperm(buff, bufc);
	    return 1;
	}
    }

    return 0;
}


FUNCTION(fun_link)
{
    if (check_command(player, "@link", buff, bufc))
	return;
    do_link(player, cause, 0, fargs[0], fargs[1]);
}

FUNCTION(fun_tel)
{
    if (check_command(player, "@teleport", buff, bufc))
	return;
    do_teleport(player, cause, 0, fargs[0], fargs[1]);
}

FUNCTION(fun_wipe)
{
    if (check_command(player, "@wipe", buff, bufc))
	return;
    do_wipe(player, cause, 0, fargs[0]);
}

FUNCTION(fun_pemit)
{
    if (check_command(player, "@pemit", buff, bufc))
	return;
    do_pemit_list(player, fargs[0], fargs[1], 0);
}

FUNCTION(fun_remit)
{
    if (check_command(player, "@pemit", buff, bufc))
	return;
    do_pemit_list(player, fargs[0], fargs[1], 1);
}

FUNCTION(fun_force)
{
    if (check_command(player, "@force", buff, bufc))
	return;
    do_force(player, cause, 0, fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_trigger)
{
	if (nfargs < 1) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	if (check_command(player, "@trigger", buff, bufc))
	    return;
	do_trigger(player, cause, 0, fargs[0], &(fargs[1]), nfargs - 1);
}

FUNCTION(fun_wait)
{
    do_wait(player, cause, 0, fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_command)
{
    CMDENT *cmdp;
    char tbuf1[1], tbuf2[1];
    int key;

    if (!fargs[0] || !*fargs[0])
	return;

    cmdp = (CMDENT *) hashfind(fargs[0], &mudstate.command_htab);
    if (!cmdp) {
	notify(player, "Command not found.");
	return;
    }

    if (Invalid_Objtype(player) || !check_access(player, cmdp->perms) ||
	(!Builder(player) && Protect(CA_GBL_BUILD) &&
	 !(mudconf.control_flags & CF_BUILD))) {
	notify(player, "Permission denied.");
	return;
    }

    if (!(cmdp->callseq & CS_FUNCTION) || (cmdp->callseq & CS_ADDED)) {
	notify(player, "Cannot call that command.");
	return;
    }

    /* Strip command flags that are irrelevant. */

    key = cmdp->extra;
    key &= ~(SW_GOT_UNIQUE | SW_MULTIPLE | SW_NOEVAL);

    /* Can't handle null args, so make sure there's something there. */

    tbuf1[0] = '\0';
    tbuf2[0] = '\0';

    switch (cmdp->callseq & CS_NARG_MASK) {
	case CS_NO_ARGS:
	    (*(cmdp->info.handler)) (player, cause, key);
	    break;
	case CS_ONE_ARG:
	    (*(cmdp->info.handler)) (player, cause, key,
				     ((fargs[1]) ? (fargs[1]) : tbuf1));
	    break;
	case CS_TWO_ARG:
	    (*(cmdp->info.handler)) (player, cause, key,
				     ((fargs[1]) ? (fargs[1]) : tbuf1),
				     ((fargs[2]) ? (fargs[2]) : tbuf2));
	    break;
	default:
	    notify(player, "Invalid command handler.");
	    return;
    }
}


/*------------------------------------------------------------------------
 * fun_create: Creates a room, thing or exit
 */

FUNCTION(fun_create)
{
	dbref thing;
	int cost;
	char sep, *name;

	varargs_preamble("CREATE", 3);
	name = fargs[0];

	if (!name || !*name) {
		safe_str("#-1 ILLEGAL NAME", buff, bufc);
		return;
	}
	if (fargs[2] && *fargs[2])
		sep = *fargs[2];
	else
		sep = 't';

	switch (sep) {
	case 'r':
		if (check_command(player, "@dig", buff, bufc)) {
			return;
		}
		thing = create_obj(player, TYPE_ROOM, name, 0);
		break;
	case 'e':
		if (check_command(player, "@open", buff, bufc)) {
			return;
		}
		thing = create_obj(player, TYPE_EXIT, name, 0);
		if (thing != NOTHING) {
			s_Exits(thing, player);
			s_Next(thing, Exits(player));
			s_Exits(player, thing);
		}
		break;
	default:
		if (check_command(player, "@create", buff, bufc)) {
			return;
		}
		if (fargs[1] && *fargs[1]) {
		    cost = atoi(fargs[1]);
		    if (cost < mudconf.createmin || cost > mudconf.createmax) {
			safe_str("#-1 COST OUT OF RANGE", buff, bufc);
			return;
		    }
		} else {
		    cost = mudconf.createmin;
		}
		thing = create_obj(player, TYPE_THING, name, cost);
		if (thing != NOTHING) {
			move_via_generic(thing, player, NOTHING, 0);
			s_Home(thing, new_home(player));
		}
		break;
	}
	safe_dbref(buff, bufc, thing);
}

/*---------------------------------------------------------------------------
 * fun_set: sets an attribute on an object
 */

static void set_attr_internal(player, thing, attrnum, attrtext, key, buff, bufc)
dbref player, thing;
int attrnum, key;
char *attrtext, *buff;
char **bufc;
{
	dbref aowner;
	int aflags, could_hear;
	ATTR *attr;

	attr = atr_num(attrnum);
	atr_pget_info(thing, attrnum, &aowner, &aflags);
	if (attr && Set_attr(player, thing, attr, aflags)) {
		if ((attr->check != NULL) &&
		    (!(*attr->check) (0, player, thing, attrnum, attrtext))) {
		        safe_noperm(buff, bufc);
			return;
		}
		could_hear = Hearer(thing);
		atr_add(thing, attrnum, attrtext, Owner(player), aflags);
		handle_ears(thing, could_hear, Hearer(thing));
		if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing))
			notify_quiet(player, "Set.");
	} else {
		safe_str("#-1 PERMISSION DENIED.", buff, bufc);
	}
}

FUNCTION(fun_set)
{
	dbref thing, thing2, aowner;
	char *p, *buff2;
	int atr, atr2, aflags, alen, clear, flagvalue, could_hear;
	ATTR *attr, *attr2;

	/* obj/attr form? */

	if (check_command(player, "@set", buff, bufc))
	    return;

	if (parse_attrib(player, fargs[0], &thing, &atr)) {
		if (atr != NOTHING) {

			/* must specify flag name */

			if (!fargs[1] || !*fargs[1]) {

				safe_str("#-1 UNSPECIFIED PARAMETER", buff, bufc);
			}
			/* are we clearing? */

			clear = 0;
			if (*fargs[0] == NOT_TOKEN) {
				fargs[0]++;
				clear = 1;
			}
			/* valid attribute flag? */

			flagvalue = search_nametab(player,
					indiv_attraccess_nametab, fargs[1]);
			if (flagvalue < 0) {
				safe_str("#-1 CAN NOT SET", buff, bufc);
				return;
			}
			/* make sure attribute is present */

			if (!atr_get_info(thing, atr, &aowner, &aflags)) {
				safe_str("#-1 ATTRIBUTE NOT PRESENT ON OBJECT", buff, bufc);
				return;
			}
			/* can we write to attribute? */

			attr = atr_num(atr);
			if (!attr || !Set_attr(player, thing, attr, aflags)) {
				safe_noperm(buff, bufc);
				return;
			}
			/* just do it! */

			if (clear)
				aflags &= ~flagvalue;
			else
				aflags |= flagvalue;
			could_hear = Hearer(thing);
			atr_set_flags(thing, atr, aflags);

			return;
		}
	}
	/* find thing */

	if ((thing = match_controlled(player, fargs[0])) == NOTHING) {
		safe_nothing(buff, bufc);
		return;
	}
	/* check for attr set first */
	for (p = fargs[1]; *p && (*p != ':'); p++) ;

	if (*p) {
		*p++ = 0;
		atr = mkattr(fargs[1]);
		if (atr <= 0) {
			safe_str("#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc);
			return;
		}
		attr = atr_num(atr);
		if (!attr) {
			safe_noperm(buff, bufc);
			return;
		}
		atr_get_info(thing, atr, &aowner, &aflags);
		if (!Set_attr(player, thing, attr, aflags)) {
			safe_noperm(buff, bufc);
			return;
		}
		buff2 = alloc_lbuf("fun_set");

		/* check for _ */
		if (*p == '_') {
			strcpy(buff2, p + 1);
			if (!parse_attrib(player, p + 1, &thing2, &atr2) ||
			    (atr == NOTHING)) {
				free_lbuf(buff2);
				safe_nomatch(buff, bufc);
				return;
			}
			attr2 = atr_num(atr);
			p = buff2;
			atr_pget_str(buff2, thing2, atr2, &aowner,
				     &aflags, &alen);

			if (!attr2 ||
			 !See_attr(player, thing2, attr2, aowner, aflags)) {
				free_lbuf(buff2);
				safe_noperm(buff, bufc);
				return;
			}
		}
		/* set it */

		set_attr_internal(player, thing, atr, p, 0, buff, bufc);
		free_lbuf(buff2);
		return;
	}
	/* set/clear a flag */
	flag_set(thing, player, fargs[1], 0);
}
