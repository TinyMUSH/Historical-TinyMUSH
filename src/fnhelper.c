/* fnhelper.c - helper functions for MUSH functions */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "functions.h"	/* required by code */
#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "ansi.h"	/* required by code */

/* ---------------------------------------------------------------------------
 * Trim off leading and trailing spaces if the separator char is a space
 */

char *trim_space_sep(str, sep, sep_len)
char *str;
Delim sep;
int sep_len;
{
	char *p;

	if ((sep_len > 1) || (sep.c != ' ') || !*str)
		return str;
	while (*str == ' ')
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
 * split_token: Get next token from string as null-term string.  String is
 *              destructively modified.
 */

char *next_token(str, sep, sep_len)
char *str;
Delim sep;
int sep_len;
{
    char *p;

    if (sep_len == 1) {
	while (*str && (*str != sep.c))
		str++;
	if (!*str)
		return NULL;
	str++;
	if (sep.c == ' ') {
		while (*str == sep.c)
			str++;
	}
    } else {
	if ((p = strstr(str, sep.str)) == NULL)
	    return NULL;
	str = (char *)(p + sep_len);
    }
    return str;
}

char *split_token(sp, sep, sep_len)
char **sp;
Delim sep;
int sep_len;
{
	char *str, *save, *p;

	save = str = *sp;
	if (!str) {
		*sp = NULL;
		return NULL;
	}
	if (sep_len == 1) {
	    while (*str && (*str != sep.c))
		str++;
	    if (*str) {
		*str++ = '\0';
		if (sep.c == ' ') {
			while (*str == sep.c)
				str++;
		}
	    } else {
		str = NULL;
	    }
	} else {
	    if ((p = strstr(str, sep.str)) != NULL) {
		*p = '\0';
		str = (char *)(p + sep_len);
	    } else {
		str = NULL;
	    }
	}
	*sp = str;
	return save;
}

/* ---------------------------------------------------------------------------
 * Count the words in a delimiter-separated list.
 */

int countwords(str, sep, sep_len)
char *str;
Delim sep;
int sep_len;
{
	int n;

	str = trim_space_sep(str, sep, sep_len);
	if (!*str)
		return 0;
	for (n = 0; str; str = next_token(str, sep, sep_len), n++) ;
	return n;
}

/* ---------------------------------------------------------------------------
 * list2arr, arr2list: Convert lists to arrays and vice versa.
 */

int list2arr(arr, maxlen, list, sep, sep_len)
char *arr[], *list;
Delim sep;
int maxlen, sep_len;
{
	char *p;
	int i;

	list = trim_space_sep(list, sep, sep_len);
	p = split_token(&list, sep, sep_len);
	for (i = 0; p && i < maxlen;
	     i++, p = split_token(&list, sep, sep_len)) {
		arr[i] = p;
	}
	return i;
}

void arr2list(arr, alen, list, bufc, sep, sep_len)
char *arr[], **bufc, *list;
Delim sep;
int alen, sep_len;
{
	int i;

	if (alen) {
	    safe_str(arr[0], list, bufc);
	}
	for (i = 1; i < alen; i++) {
	    print_sep(sep, sep_len, list, bufc);
	    safe_str(arr[i], list, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * Find the ANSI states at the end of each word of a list.
 * Note that list2ansi() function malloc()s memory which must be freed.
 */

static char *split_ansi_state(sp, sep, ansi_state, asp)
char **sp, sep, *ansi_state, **asp;
{
	char *str, *save, *endp, *p;
	int i;

	/* Note: Anything that calls this function MUST check for
	 * sep being ESC_CHAR or ANSI_END, which should be illegal
	 * separators here.
	 */

	save = str = *sp;
	if (!str) {
		*sp = NULL;
		return NULL;
	}

	while (*str && (*str != sep)) {
	    if (*str == ESC_CHAR) {
		endp = strchr(str, ANSI_END);
		if (endp) {
		    /* Between p and endp is a set of numbers separated by
		     * semicolons.
		     */
		    *endp = '\0';
		    str += 2;
		    do {
			p = strchr(str, ';');
			if (p)
			    *p++ = '\0';
			i = atoi(str);
			if (i == 0) {
			    *asp = ansi_state;
			    **asp = '\0';
			} else if ((i < I_ANSI_NUM) && ansi_lettab[i]) {
			    **asp = ansi_lettab[i];
			    (*asp)++;
			}
			str = p;
		    } while (p && *p);
		    **asp = '\0';
		    str = endp;
		}
	    }
	    str++;
	}

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

int list2ansi(arr, prior_state, maxlen, list, sep)
    char *arr[], *prior_state, *list;
    Delim sep;
    int maxlen;
{
    int i;
    char *p, *asp;
    static char ansi_status[LBUF_SIZE / 2];

    if (*prior_state) {
	strcpy(ansi_status, prior_state);
	asp = ansi_status + strlen(prior_state);
    } else {
	ansi_status[0] = '\0';
	asp = ansi_status;
    }
    list = trim_space_sep(list, sep, 1);
    p = split_ansi_state(&list, sep.c, ansi_status, &asp);
    for (i = 0; p && (i < maxlen);
	 i++, p = split_ansi_state(&list, sep.c, ansi_status, &asp)) {
	arr[i] = XSTRDUP(ansi_status, "list2ansi");
    }
    return i;
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

int delim_check(fargs, nfargs, sep_arg, sep, buff, bufc,
		player, caller, cause, cargs, ncargs, dflags)
char *fargs[], *cargs[], *buff, **bufc;
Delim *sep;
int nfargs, ncargs, sep_arg, dflags;
dbref player, caller, cause;
{
    char *tstr, *bp, *str;
    int tlen, retcode;

    if (nfargs < sep_arg) {
	(*sep).c = ' ';
	return 1;
    }

    tlen = strlen(fargs[sep_arg - 1]);
    if (tlen <= 1)
	dflags &= ~DELIM_EVAL;

    if (dflags & DELIM_EVAL) {
	tstr = bp = alloc_lbuf("delim_check");
	str = fargs[sep_arg - 1];
	exec(tstr, &bp, player, caller, cause,
	     EV_EVAL | EV_FCHECK, &str, cargs, ncargs);
	*bp = '\0';
	tlen = strlen(tstr);
    } else {
	tstr = fargs[sep_arg - 1];
    }

    retcode = 1;
    if (tlen == 0) {
	(*sep).c = ' ';
    } else if (tlen == 1) {
	(*sep).c = *tstr;
    } else {
	if ((dflags & DELIM_NULL) &&
	    !strcmp(tstr, (char *) NULL_DELIM_VAR)) {
	    (*sep).c = '\0';
	} else if ((dflags & DELIM_CRLF) &&
		   !strcmp(tstr, (char *) "\r\n")) {
	    (*sep).c = '\r';
	} else if (dflags & DELIM_STRING) {
	    if (tlen > MAX_DELIM_LEN) {
		safe_str("#-1 SEPARATOR TOO LONG", buff, bufc);
		retcode = 0;
	    } else {
		strcpy((*sep).str, tstr);
		retcode = tlen;
	    }
	} else {
	    safe_str("#-1 SEPARATOR MUST BE ONE CHARACTER",
		     buff, bufc);
	    retcode = 0;
	}
    }

    if (dflags & DELIM_EVAL)
	free_lbuf(tstr);

    return (retcode);
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
	temp2 = Eat_Spaces(arg);
	if (!*temp2)
		return 0;
	if (is_integer(temp2))
		return atoi(temp2);
	return 1;
}

/* ---------------------------------------------------------------------------
 * used by fun_reverse and fun_revwords to reverse things
 */

INLINE void do_reverse(from, to)
    char *from, *to;
{
    char *tp;

    tp = to + strlen(from);
    *tp-- = '\0';
    while (*from) {
	*tp-- = *from++;
    }
}
