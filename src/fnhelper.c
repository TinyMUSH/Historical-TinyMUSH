/* fnhelper.c -- helper functions for MUSH functions */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "fnhelper.h"

/* ---------------------------------------------------------------------------
 * Trim off leading and trailing spaces if the separator char is a space
 */

char *trim_space_sep(str, sep)
char *str, sep;
{
	char *p;

	if (sep != ' ')
		return str;
	while (*str && (*str == ' '))
		str++;
	for (p = str; *p; p++) ;
	for (p--; *p == ' ' && p > str; p--) ;
	p++;
	*p = '\0';
	return str;
}

/* ---------------------------------------------------------------------------
 * Tokenizer functions.
 * next_token: Point at start of next token in string
/* split_token: Get next token from string as null-term string.  String is
 *              destructively modified.
 */

char *next_token(str, sep)
char *str, sep;
{
	while (*str && (*str != sep))
		str++;
	if (!*str)
		return NULL;
	str++;
	if (sep == ' ') {
		while (*str == sep)
			str++;
	}
	return str;
}

char *split_token(sp, sep)
char **sp, sep;
{
	char *str, *save;

	save = str = *sp;
	if (!str) {
		*sp = NULL;
		return NULL;
	}
	while (*str && (*str != sep))
		str++;
	if (*str) {
		*str++ = '\0';
		if (sep == ' ') {
			while (*str == sep)
				str++;
		}
	} else {
		str = NULL;
	}
	*sp = str;
	return save;
}

/* ---------------------------------------------------------------------------
 * Count the words in a delimiter-separated list.
 */

int countwords(str, sep)
char *str, sep;
{
	int n;

	str = trim_space_sep(str, sep);
	if (!*str)
		return 0;
	for (n = 0; str; str = next_token(str, sep), n++) ;
	return n;
}

/* ---------------------------------------------------------------------------
 * list2arr, arr2list: Convert lists to arrays and vice versa.
 */

int list2arr(arr, maxlen, list, sep)
char *arr[], *list, sep;
int maxlen;
{
	char *p;
	int i;

	list = trim_space_sep(list, sep);
	p = split_token(&list, sep);
	for (i = 0; p && i < maxlen; i++, p = split_token(&list, sep)) {
		arr[i] = p;
	}
	return i;
}

void arr2list(arr, alen, list, bufc, sep)
char *arr[], **bufc, *list, sep;
int alen;
{
	int i;

	if (alen) {
	    safe_str(arr[0], list, bufc);
	}
	for (i = 1; i < alen; i++) {
	    print_sep(sep, list, bufc);
	    safe_str(arr[i], list, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * Quick-matching for function purposes.
 */

dbref match_thing(player, name)
dbref player;
char *name;
{
	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	return (noisy_match_result());
}

/* ---------------------------------------------------------------------------
 * Random number generator. This uses Whip's implementation of an algorithm
 * in the _Communications of the ACM_, Volume 31, Number 10, from the article
 * "Random Number Generators: Good Ones are Hard to Find" (S.K. Park,
 * K.W. Miller).
 */

double makerandom()
{
    /* An int must be at least 32 bits. Don't change these constants. */

    const unsigned int a = 16807;
    const unsigned int m = 2147482647;
    const unsigned int q = 127773; /* m div a */
    const unsigned int r = 2836;   /* m mod a */

    unsigned int lo, hi;
    int test;
    static unsigned int seed = 0;
  
    /* This isn't the best seed in the world, but it's portable. There's
     * nothing truly random that's portable to get to, unfortunately. We're
     * going to adjust with the PID because any normal user can get the time
     * the MUSH started (or close to it) trivially, which would make the whole
     * sequence predictable. Using PID isn't much better, but it's portable, 
     * and means you at least have to have machine access to figure it out
     * (or be a wizard).
     */

    if (!seed) seed = time(NULL) + (int) getpid();

    hi = seed / q; 
    lo = seed % q;
  
    test = (a * lo) - (r * hi);

    if (test > 0) {
	seed = test;
    } else {
	seed = test + m;
    }

    return ((double) seed / m);
}

/* ---------------------------------------------------------------------------
 * fn_range_check: Check # of args to a function with an optional argument
 * for validity.
 */

int fn_range_check(fname, nfargs, minargs, maxargs, result, bufc)
const char *fname;
char *result, **bufc;
int nfargs, minargs, maxargs;
{
	if ((nfargs >= minargs) && (nfargs <= maxargs))
		return 1;

	if (maxargs == (minargs + 1))
		safe_tprintf_str(result, bufc, "#-1 FUNCTION (%s) EXPECTS %d OR %d ARGUMENTS",
				 fname, minargs, maxargs);
	else
		safe_tprintf_str(result, bufc, "#-1 FUNCTION (%s) EXPECTS BETWEEN %d AND %d ARGUMENTS",
				 fname, minargs, maxargs);
	return 0;
}

/* ---------------------------------------------------------------------------
 * delim_check: obtain delimiter
 */

int delim_check(fargs, nfargs, sep_arg, sep, buff, bufc, eval, player, cause,
		cargs, ncargs, allow_special)
char *fargs[], *cargs[], *sep, *buff, **bufc;
int nfargs, ncargs, sep_arg, eval, allow_special;
dbref player, cause;
{
	char *tstr, *bp, *str;
	int tlen;

	if (nfargs >= sep_arg) {
		tlen = strlen(fargs[sep_arg - 1]);
		if (tlen <= 1)
			eval = 0;
		if (eval) {
			tstr = bp = alloc_lbuf("delim_check");
			str = fargs[sep_arg - 1];
			exec(tstr, &bp, 0, player, cause, EV_EVAL | EV_FCHECK,
			     &str, cargs, ncargs);
			*bp = '\0';
			if (allow_special &&
			    !strcmp(tstr, (char *) NULL_DELIM_VAR)) {
			    *sep = '\0';
			    tlen = 1;
			} else if (allow_special &&
				   !strcmp(tstr, (char *) "\r\n")) {
			    *sep = '\r';
			    tlen = 1;
			} else {
			    tlen = strlen(tstr);
			    *sep = *tstr;
			}
			free_lbuf(tstr);
		}
		if (tlen == 0) {
			*sep = ' ';
		} else if (allow_special && !eval && (tlen == 2) &&
			   !strcmp(fargs[sep_arg - 1],
				   (char *) NULL_DELIM_VAR)) {
		        *sep = '\0';
		} else if (allow_special && !eval && (tlen == 2) &&
			   !strcmp(fargs[sep_arg - 1], (char *) "\r\n")) {
		        *sep = '\r';
		} else if (tlen != 1) {
			safe_str("#-1 SEPARATOR MUST BE ONE CHARACTER",
				 buff, bufc);
			return 0;
		} else if (!eval) {
			*sep = *fargs[sep_arg - 1];
		}
	} else {
		*sep = ' ';
	}
	return 1;
}

/* ---------------------------------------------------------------------------
 * Check to make sure that we can read an attribute off an object.
 */

int check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)
dbref player, thing;
ATTR *attr;
int aowner, aflags;
char *buff, **bufc;
{
	int see_it;

	/* If we have explicit read permission to the attr, return it */

	if (See_attr_explicit(player, thing, attr, aowner, aflags))
		return 1;

	/* If we are nearby or have examine privs to the attr and it is 
	 * visible to us, return it. 
	 */

	see_it = See_attr(player, thing, attr, aowner, aflags);
	if ((Examinable(player, thing) || nearby(player, thing) || See_All(player)) && see_it)
		return 1;

	/* For any object, we can read its visible attributes, EXCEPT for
	 * descs, which are only visible if read_rem_desc is on. 
	 */

	if (see_it) {
		if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
			safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
			return 0;
		} else {
			return 1;
		}
	}
	safe_noperm(buff, bufc);
	return 0;
}

/* ---------------------------------------------------------------------------
 * Boolean true/false check.
 */

int xlate(arg)
char *arg;
{
	char *temp2;

	if (arg[0] == '#') {
		arg++;
		if (is_integer(arg)) {
		    if (mudconf.bools_oldstyle) {
			switch (atoi(arg)) {
			    case -1:
				return 0;
			    case 0:
				return 0;
			    default:
				return 1;
			}
		    } else {
			return (atoi(arg) >= 0);
		    }
		}
		if (mudconf.bools_oldstyle) {
		    return 0;
		} else {
		    /* Case of '#-1 <string>' */
		    return !((arg[0] == '-') && (arg[1] == '1') &&
			     (arg[2] == ' '));
		}
	}
	temp2 = trim_space_sep(arg, ' ');
	if (!*temp2)
		return 0;
	if (is_integer(temp2))
		return atoi(temp2);
	return 1;
}
