/* functions.c - MUSH function handlers */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <limits.h>
#include <math.h>

#include "externs.h"
#include "misc.h"
#include "attrs.h"
#include "match.h"
#include "command.h"
#include "functions.h"
#include "ansi.h"

#ifdef FLOATING_POINTS
#ifndef linux                   /* linux defines atof as a macro */
double atof();
#endif                          /* ! linux */
#define aton atof
typedef double NVAL;
#else
#define aton atoi
typedef int NVAL;
#endif                          /* FLOATING_POINTS */

UFUN *ufun_head;

static char space_buffer[LBUF_SIZE];

extern NAMETAB indiv_attraccess_nametab[];

extern void FDECL(cf_log_notfound, (dbref player, char *cmd,
				    const char *thingname, char *thing));

extern dbref FDECL(get_programmer, (dbref));
extern char * FDECL(get_doing, (dbref));
extern dbref FDECL(find_connected_ambiguous, (dbref, char *));

extern INLINE int FDECL(safe_chr_real_fn, (char, char *, char **, int));

/* Function definitions from funceval.c */

#define	XFUNCTION(x)	\
	extern void x();

#ifdef USE_COMSYS
XFUNCTION(fun_cwho);
#endif
XFUNCTION(fun_beep);
XFUNCTION(fun_ansi);
XFUNCTION(fun_zone);
XFUNCTION(fun_link);
XFUNCTION(fun_tel);
XFUNCTION(fun_pemit);
XFUNCTION(fun_remit);
XFUNCTION(fun_force);
XFUNCTION(fun_trigger);
XFUNCTION(fun_wait);
XFUNCTION(fun_create);
XFUNCTION(fun_set);
XFUNCTION(fun_wipe);
XFUNCTION(fun_command);
XFUNCTION(fun_last);
XFUNCTION(fun_matchall);
XFUNCTION(fun_ports);
XFUNCTION(fun_mix);
XFUNCTION(fun_step);
XFUNCTION(fun_foreach);
XFUNCTION(fun_munge);
XFUNCTION(fun_visible);
XFUNCTION(fun_elements);
XFUNCTION(fun_grab);
XFUNCTION(fun_scramble);
XFUNCTION(fun_shuffle);
XFUNCTION(fun_sortby);
XFUNCTION(fun_default);
XFUNCTION(fun_edefault);
XFUNCTION(fun_udefault);
XFUNCTION(fun_findable);
XFUNCTION(fun_hasattr);
XFUNCTION(fun_hasattrp);
XFUNCTION(fun_zwho);
XFUNCTION(fun_inzone);
XFUNCTION(fun_children);
XFUNCTION(fun_encrypt);
XFUNCTION(fun_decrypt);
XFUNCTION(fun_objeval);
XFUNCTION(fun_localize);
XFUNCTION(fun_null);
XFUNCTION(fun_squish);
XFUNCTION(fun_stripansi);
XFUNCTION(fun_zfun);
XFUNCTION(fun_columns);
XFUNCTION(fun_playmem);
XFUNCTION(fun_objmem);
XFUNCTION(fun_orflags);
XFUNCTION(fun_andflags);
XFUNCTION(fun_strtrunc);
XFUNCTION(fun_ifelse);
XFUNCTION(fun_nonzero);
XFUNCTION(fun_inc);
XFUNCTION(fun_dec)
#ifdef USE_MAIL
XFUNCTION(fun_mail);
XFUNCTION(fun_mailfrom);
#endif
XFUNCTION(fun_die);
XFUNCTION(fun_lit);
XFUNCTION(fun_shl);
XFUNCTION(fun_shr);
XFUNCTION(fun_band);
XFUNCTION(fun_bor);
XFUNCTION(fun_bnand);
XFUNCTION(fun_strcat);
XFUNCTION(fun_grep);
XFUNCTION(fun_grepi);
XFUNCTION(fun_art);
XFUNCTION(fun_alphamax);
XFUNCTION(fun_alphamin);
XFUNCTION(fun_valid);
XFUNCTION(fun_hastype);
XFUNCTION(fun_lparent);
XFUNCTION(fun_empty);
XFUNCTION(fun_push);
XFUNCTION(fun_peek);
XFUNCTION(fun_pop);
XFUNCTION(fun_toss);
XFUNCTION(fun_items);
XFUNCTION(fun_lstack);
XFUNCTION(fun_dup);
XFUNCTION(fun_popn);
XFUNCTION(fun_swap);
XFUNCTION(fun_x);
XFUNCTION(fun_setx);
XFUNCTION(fun_xvars);
XFUNCTION(fun_let);
XFUNCTION(fun_lvars);
XFUNCTION(fun_clearvars);
XFUNCTION(fun_regparse);
XFUNCTION(fun_regmatch);
XFUNCTION(fun_translate);
XFUNCTION(fun_lastcreate);
XFUNCTION(fun_structure);
XFUNCTION(fun_construct);
XFUNCTION(fun_load);
XFUNCTION(fun_unload);
XFUNCTION(fun_destruct);
XFUNCTION(fun_unstructure);
XFUNCTION(fun_z);
XFUNCTION(fun_modify);
XFUNCTION(fun_lstructures);
XFUNCTION(fun_linstances);
XFUNCTION(fun_sql);

#ifdef PUEBLO_SUPPORT
XFUNCTION(fun_html_escape);
XFUNCTION(fun_html_unescape);
XFUNCTION(fun_url_escape);
XFUNCTION(fun_url_unescape);
#endif /* PUEBLO SUPPORT */

#ifdef TCL_INTERP_SUPPORT
XFUNCTION(fun_tclclear);
XFUNCTION(fun_tcleval);
XFUNCTION(fun_tclmodule);
XFUNCTION(fun_tclparams);
XFUNCTION(fun_tclregs);
#endif /* TCL_INTERP_SUPPORT */

/* This is the prototype for functions */

#define	FUNCTION(x)	\
	static void x(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs) \
	char *buff, **bufc; \
	dbref player, cause; \
	char *fargs[], *cargs[]; \
	int nfargs, ncargs;


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

/* next_token: Point at start of next token in string */

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

/* split_token: Get next token from string as null-term string.  String is
 * destructively modified.
 */

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

dbref match_thing(player, name)
dbref player;
char *name;
{
	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	return (noisy_match_result());
}

/* ---------------------------------------------------------------------------
 * List management utilities.
 */

#define	ALPHANUM_LIST	1
#define	NUMERIC_LIST	2
#define	DBREF_LIST	3
#define	FLOAT_LIST	4

static int autodetect_list(ptrs, nitems)
char *ptrs[];
int nitems;
{
	int sort_type, i;
	char *p;

	sort_type = NUMERIC_LIST;
	for (i = 0; i < nitems; i++) {
		switch (sort_type) {
		case NUMERIC_LIST:
			if (!is_number(ptrs[i])) {

				/* If non-numeric, switch to alphanum sort.
				 * Exception: if this is the first element 
				 * and it is a good dbref, switch to a dbref
				 * sort. We're a little looser than the
				 * normal 'good dbref' rules, any number
				 * following the #-sign is  accepted.  
				 */

				if (i == 0) {
					p = ptrs[i];
					if (*p++ != NUMBER_TOKEN) {
						return ALPHANUM_LIST;
					} else if (is_integer(p)) {
						sort_type = DBREF_LIST;
					} else {
						return ALPHANUM_LIST;
					}
				} else {
					return ALPHANUM_LIST;
				}
			} else if (index(ptrs[i], '.')) {
				sort_type = FLOAT_LIST;
			}
			break;
		case FLOAT_LIST:
			if (!is_number(ptrs[i])) {
				sort_type = ALPHANUM_LIST;
				return ALPHANUM_LIST;
			}
			break;
		case DBREF_LIST:
			p = ptrs[i];
			if (*p++ != NUMBER_TOKEN)
				return ALPHANUM_LIST;
			if (!is_integer(p))
				return ALPHANUM_LIST;
			break;
		default:
			return ALPHANUM_LIST;
		}
	}
	return sort_type;
}

static int get_list_type(fargs, nfargs, type_pos, ptrs, nitems)
char *fargs[], *ptrs[];
int nfargs, nitems, type_pos;
{
	if (nfargs >= type_pos) {
		switch (ToLower(*fargs[type_pos - 1])) {
		case 'd':
			return DBREF_LIST;
		case 'n':
			return NUMERIC_LIST;
		case 'f':
			return FLOAT_LIST;
		case '\0':
			return autodetect_list(ptrs, nitems);
		default:
			return ALPHANUM_LIST;
		}
	}
	return autodetect_list(ptrs, nitems);
}

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

static int dbnum(dbr)
char *dbr;
{
	if ((*dbr != '#') || (strlen(dbr) < 2))
		return 0;
	else
		return atoi(dbr + 1);
}

/* ---------------------------------------------------------------------------
 * nearby_or_control: Check if player is near or controls thing
 */

int nearby_or_control(player, thing)
dbref player, thing;
{
	if (!Good_obj(player) || !Good_obj(thing))
		return 0;
	if (Controls(player, thing))
		return 1;
	if (!nearby(player, thing))
		return 0;
	return 1;
}
/* ---------------------------------------------------------------------------
 * fval: copy the floating point value into a buffer and make it presentable
 */

#ifdef FLOATING_POINTS
static void fval(buff, bufc, result)
char *buff, **bufc;
double result;
{
	char *p, *buf1;

	buf1 = *bufc;
	safe_tprintf_str(buff, bufc, "%.6f", result);	/* get double val
							 * into buffer 
							 */
	**bufc = '\0';
	p = (char *)rindex(buf1, '0');
	if (p == NULL) {	/* remove useless trailing 0's */
		return;
	} else if (*(p + 1) == '\0') {
		while (*p == '0') {
			*p-- = '\0';
		}
		*bufc = p + 1;
	}
	p = (char *)rindex(buf1, '.');	/* take care of dangling '.' */

	if ((p != NULL) && (*(p + 1) == '\0')) {
			*p = '\0';
		*bufc = p;
	}
}
#else
#define fval(b,p,n)  safe_ltos(b,p,n)
#endif

static void fval_buf(buff, result)
    char *buff;
    double result;
{
    char *p;

    sprintf(buff, "%.6f", result);      /* get double val into buffer */

    /* remove useless trailing 0's */
    if ((p = (char *) rindex(buff, '0')) == NULL)
        return;
    else if (*(p + 1) == '\0') {
        while (*p == '0')
            *p-- = '\0';
    }

    p = (char *) rindex(buff, '.');     /* take care of dangling '.' */
    if (*(p + 1) == '\0')
        *p = '\0';
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
 * fun_words: Returns number of words in a string.
 * Added 1/28/91 Philip D. Wasson
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

FUNCTION(fun_words)
{
	char sep;

	if (nfargs == 0) {
		safe_chr('0', buff, bufc);
		return;
	}
	varargs_preamble("WORDS", 2);
	safe_ltos(buff, bufc, countwords(fargs[0], sep));
}

/* ------------------------------------------------------------------------
 * fun_flags: Returns the flags on an object.
 * Because @switch is case-insensitive, not quite as useful as it could be.
 */

FUNCTION(fun_flags)
{
	dbref it;
	char *buff2;

	it = match_thing(player, fargs[0]);
	if ((it != NOTHING) &&
	    (mudconf.pub_flags || Examinable(player, it) || (it == cause))) {
		buff2 = unparse_flags(player, it);
		safe_str(buff2, buff, bufc);
		free_sbuf(buff2);
	} else
		safe_nothing(buff, bufc);
	return;
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
 * fun_abs: Returns the absolute value of its argument.
 */

FUNCTION(fun_abs)
{
#ifdef FLOATING_POINTS
	double num;

	num = atof(fargs[0]);
	if (num == 0.0) {
		safe_chr('0', buff, bufc);
	} else if (num < 0.0) {
		fval(buff, bufc, -num);
	} else {
		fval(buff, bufc, num);
	}
#else
	ltos(buff, abs(atoi(fargs[0])));
#endif
}

/* ---------------------------------------------------------------------------
 * fun_sign: Returns -1, 0, or 1 based on the the sign of its argument.
 */

FUNCTION(fun_sign)
{
	NVAL num;

	num = aton(fargs[0]);
	if (num < 0)
		safe_known_str("-1", 2, buff, bufc);
	else if (num > 0) {
		safe_chr('1', buff, bufc);
	} else {
		safe_chr('0', buff, bufc);
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

/* ---------------------------------------------------------------------------
 * fun_get, fun_get_eval: Get attribute from object.
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

FUNCTION(fun_get)
{
	dbref thing, aowner;
	int attrib, free_buffer, aflags, alen;
	ATTR *attr;
	char *atr_gotten;
	struct boolexp *bool;

	if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (attrib == NOTHING) {
		return;
	}
	free_buffer = 1;
	attr = atr_num(attrib);	/* We need the attr's flags for this: */
	if (!attr) {
		return;
	}
	if (attr->flags & AF_IS_LOCK) {
		atr_gotten = atr_get(thing, attrib, &aowner, &aflags, &alen);
		if (Read_attr(player, thing, attr, aowner, aflags)) {
			bool = parse_boolexp(player, atr_gotten, 1);
			free_lbuf(atr_gotten);
			atr_gotten = unparse_boolexp(player, bool);
			free_boolexp(bool);
		} else {
			free_lbuf(atr_gotten);
			atr_gotten = (char *)"#-1 PERMISSION DENIED";
		}
		free_buffer = 0;
	} else {
		atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
	}

	/* Perform access checks.  c_r_p fills buff with an error message
	 * if needed. 
	 */

	if (check_read_perms(player, thing, attr, aowner, aflags,
			     buff, bufc)) {
	    if (free_buffer)
		safe_known_str(atr_gotten, alen, buff, bufc);
	    else
		safe_str(atr_gotten, buff, bufc);
	}

	if (free_buffer)
		free_lbuf(atr_gotten);
	return;
}

FUNCTION(fun_xget)
{
	dbref thing, aowner;
	int attrib, free_buffer, aflags, alen;
	ATTR *attr;
	char *atr_gotten;
	struct boolexp *bool;

	if (!*fargs[0] || !*fargs[1])
		return;

	if (!parse_attrib(player, tprintf("%s/%s", fargs[0], fargs[1]),
			  &thing, &attrib)) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (attrib == NOTHING) {
		return;
	}
	free_buffer = 1;
	attr = atr_num(attrib);	/* We need the attr's flags for this: */
	if (!attr) {
		return;
	}
	if (attr->flags & AF_IS_LOCK) {
		atr_gotten = atr_get(thing, attrib, &aowner, &aflags, &alen);
		if (Read_attr(player, thing, attr, aowner, aflags)) {
			bool = parse_boolexp(player, atr_gotten, 1);
			free_lbuf(atr_gotten);
			atr_gotten = unparse_boolexp(player, bool);
			free_boolexp(bool);
		} else {
			free_lbuf(atr_gotten);
			atr_gotten = (char *)"#-1 PERMISSION DENIED";
		}
		free_buffer = 0;
	} else {
		atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
	}

	/* Perform access checks.  c_r_p fills buff with an error message  
	 * if needed. 
	 */

	if (check_read_perms(player, thing, attr, aowner, aflags,
			     buff, bufc)) {
	    if (free_buffer)
		safe_known_str(atr_gotten, alen, buff, bufc);
	    else
		safe_str(atr_gotten, buff, bufc);
	}

	if (free_buffer)
		free_lbuf(atr_gotten);
	return;
}

FUNCTION(fun_get_eval)
{
	dbref thing, aowner;
	int attrib, free_buffer, aflags, alen, eval_it;
	ATTR *attr;
	char *atr_gotten, *str;
	struct boolexp *bool;

	if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (attrib == NOTHING) {
		return;
	}
	free_buffer = 1;
	eval_it = 1;
	attr = atr_num(attrib);	/* We need the attr's flags for this: */
	if (!attr) {
		return;
	}
	if (attr->flags & AF_IS_LOCK) {
		atr_gotten = atr_get(thing, attrib, &aowner, &aflags, &alen);
		if (Read_attr(player, thing, attr, aowner, aflags)) {
			bool = parse_boolexp(player, atr_gotten, 1);
			free_lbuf(atr_gotten);
			atr_gotten = unparse_boolexp(player, bool);
			free_boolexp(bool);
		} else {
			free_lbuf(atr_gotten);
			atr_gotten = (char *)"#-1 PERMISSION DENIED";
		}
		free_buffer = 0;
		eval_it = 0;
	} else {
		atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
	}
	if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {
		if (free_buffer)
			free_lbuf(atr_gotten);
		return;
	}
	if (eval_it) {
		str = atr_gotten;
		exec(buff, bufc, 0, thing, player, EV_FIGNORE | EV_EVAL, &str,
		     (char **)NULL, 0);
	} else {
		safe_str(atr_gotten, buff, bufc);
	}
	if (free_buffer)
		free_lbuf(atr_gotten);
	return;
}

FUNCTION(fun_subeval)
{
	char *str;
	
	if (nfargs != 1) {
		safe_str("#-1 FUNCTION (EVALNOCOMP) EXPECTS 1 OR 2 ARGUMENTS", buff, bufc);
		return;
	}
	
	str = fargs[0];
	exec(buff, bufc, 0, player, cause, EV_NO_LOCATION|EV_NOFCHECK|EV_FIGNORE|EV_NO_COMPRESS,
	     &str, (char **)NULL, 0);
}	

FUNCTION(fun_eval)
{
	dbref thing, aowner;
	int attrib, free_buffer, aflags, alen, eval_it;
	ATTR *attr;
	char *atr_gotten, *str;
	struct boolexp *bool;

	if ((nfargs != 1) && (nfargs != 2)) {
		safe_str("#-1 FUNCTION (EVAL) EXPECTS 1 OR 2 ARGUMENTS", buff, bufc);
		return;
	}
	if (nfargs == 1) {
		str = fargs[0];
		exec(buff, bufc, 0, player, cause, EV_EVAL|EV_FCHECK,
		     &str, (char **)NULL, 0);
		return;
	}
	if (!*fargs[0] || !*fargs[1])
		return;

	if (!parse_attrib(player, tprintf("%s/%s", fargs[0], fargs[1]),
			  &thing, &attrib)) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (attrib == NOTHING) {
		return;
	}
	free_buffer = 1;
	eval_it = 1;
	attr = atr_num(attrib);
	if (!attr) {
		return;
	}
	if (attr->flags & AF_IS_LOCK) {
		atr_gotten = atr_get(thing, attrib, &aowner, &aflags, &alen);
		if (Read_attr(player, thing, attr, aowner, aflags)) {
			bool = parse_boolexp(player, atr_gotten, 1);
			free_lbuf(atr_gotten);
			atr_gotten = unparse_boolexp(player, bool);
			free_boolexp(bool);
		} else {
			free_lbuf(atr_gotten);
			atr_gotten = (char *)"#-1 PERMISSION DENIED";
		}
		free_buffer = 0;
		eval_it = 0;
	} else {
		atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
	}
	if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {
		if (free_buffer)
			free_lbuf(atr_gotten);
		return;
	}
	if (eval_it) {
		str = atr_gotten;
		exec(buff, bufc, 0, thing, player, EV_FIGNORE | EV_EVAL, &str,
		     (char **)NULL, 0);
	} else {
		safe_str(atr_gotten, buff, bufc);
	}
	if (free_buffer)
		free_lbuf(atr_gotten);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_u and fun_ulocal:  Call a user-defined function.
 */

static void do_ufun(buff, bufc, player, cause,
		    fargs, nfargs,
		    cargs, ncargs,
		    is_local)
char *buff, **bufc;
dbref player, cause;
char *fargs[], *cargs[];
int nfargs, ncargs, is_local;
{
	dbref aowner, thing;
	int aflags, alen, anum;
	ATTR *ap;
	char *atext, *preserve[MAX_GLOBAL_REGS], *str;
	
	/* We need at least one argument */

	if (nfargs < 1) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
		return;
	}
	/* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

	if (parse_attrib(player, fargs[0], &thing, &anum)) {
		if ((anum == NOTHING) || (!Good_obj(thing)))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(fargs[0]);
	}

	/* Make sure we got a good attribute */

	if (!ap) {
		return;
	}
	/* Use it if we can access it, otherwise return an error. */

	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!atext) {
		free_lbuf(atext);
		return;
	}
	if (!*atext) {
		free_lbuf(atext);
		return;
	}
	if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufc)) {
		free_lbuf(atext);
		return;
	}
	/* If we're evaluating locally, preserve the global registers. */

	if (is_local) {
		save_global_regs("fun_ulocal_save", preserve);
	}
	
	/* Evaluate it using the rest of the passed function args */

	str = atext;
	exec(buff, bufc, 0, thing, cause, EV_FCHECK | EV_EVAL, &str,
	     &(fargs[1]), nfargs - 1);
	free_lbuf(atext);

	/* If we're evaluating locally, restore the preserved registers. */

	if (is_local) {
		restore_global_regs("fun_ulocal_restore", preserve);
	}
}

FUNCTION(fun_u)
{
	do_ufun(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs, 0);
}

FUNCTION(fun_ulocal)
{
	do_ufun(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs, 1);
}

/* ---------------------------------------------------------------------------
 * fun_parent: Get parent of object.
 */

FUNCTION(fun_parent)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (Good_obj(it) && (Examinable(player, it) || (it == cause))) {
		safe_dbref(buff, bufc, Parent(it));
	} else {
		safe_nothing(buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

FUNCTION(fun_mid)
{
    char *s, *savep;
    int count, start, nchars, len, have_normal;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);
    len = strlen(strip_ansi(s));

    if ((start < 0) || (nchars < 0) || (start > LBUF_SIZE - 1) ||
	(nchars > LBUF_SIZE - 1)) {
	safe_str("#-1 OUT OF RANGE", buff, bufc);
	return;
    }

    if ((start >= len) || (nchars == 0))
	return;

    if (start + nchars > len)
	nchars = len - start;

    have_normal = 1;
    for (count = 0; *s && (count < start + nchars); ) {
	if (*s == ESC_CHAR) {
	    /* Start of an ANSI code. Skip to the end. */
	    savep = s;
	    while (*s && (*s != ANSI_END)) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (*s) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (!strncmp(savep, ANSI_NORMAL, 4))
		have_normal = 1;
	    else
		have_normal = 0;
	} else {
	    if (count >= start)
		safe_chr(*s, buff, bufc);
	    s++;
	    count++;
	}
    }

    if (!have_normal)
	safe_ansi_normal(buff, bufc);
}


/* ---------------------------------------------------------------------------
 * fun_first: Returns first word in a string
 */

FUNCTION(fun_first)
{
	char *s, *first, sep;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	varargs_preamble("FIRST", 2);
	s = trim_space_sep(fargs[0], sep);	/* leading spaces ... */
	first = split_token(&s, sep);
	if (first) {
		safe_str(first, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_rest: Returns all but the first word in a string 
 */


FUNCTION(fun_rest)
{
	char *s, *first, sep;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	varargs_preamble("REST", 2);
	s = trim_space_sep(fargs[0], sep);	/* leading spaces ... */
	first = split_token(&s, sep);
	if (s) {
		safe_str(s, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_left: Returns first n characters in a string
 */

FUNCTION(fun_left)
{
    char *s, *savep;
    int nchars, len, count, have_normal;

    s = fargs[0];
    nchars = atoi(fargs[1]);
    len = strlen(strip_ansi(s));

    if ((len < 1) || (nchars < 1))
	return;

    have_normal = 1;
    for (count = 0; *s && (count < nchars); ) {
	if (*s == ESC_CHAR) {
	    /* Start of an ANSI code. Skip to the end. */
	    savep = s;
	    while (*s && (*s != ANSI_END)) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (*s) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (!strncmp(savep, ANSI_NORMAL, 4))
		have_normal = 1;
	    else
		have_normal = 0;
	} else {
	    safe_chr(*s, buff, bufc);
	    s++;
	    count++;
	}
    }

    if (!have_normal)
	safe_ansi_normal(buff, bufc);
}


/* ---------------------------------------------------------------------------
 * fun_right: Returns last n characters in a string
 */

FUNCTION(fun_right)
{
    char *s, *savep;
    int nchars, start, len, count, have_normal;

    s = fargs[0];
    nchars = atoi(fargs[1]);
    len = strlen(strip_ansi(s));
    start = len - nchars;

    if ((len < 1) || (nchars < 1))
	return;

    if (nchars > len)
	start = 0;
    else
	start = len - nchars;

    have_normal = 1;
    for (count = 0; *s && (count < start + nchars); ) {
	if (*s == ESC_CHAR) {
	    /* Start of an ANSI code. Skip to the end. */
	    savep = s;
	    while (*s && (*s != ANSI_END)) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (*s) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (!strncmp(savep, ANSI_NORMAL, 4))
		have_normal = 1;
	    else
		have_normal = 0;
	} else {
	    if (count >= start)
		safe_chr(*s, buff, bufc);
	    s++;
	    count++;
	}
    }

    if (!have_normal)
	safe_ansi_normal(buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_chomp: If the line ends with a newline ('\r\n'), chop it off.
 */

FUNCTION(fun_chomp)
{
    int len = strlen(fargs[0]);
    if ((fargs[0][len - 2] == '\r') && (fargs[0][len - 1] == '\n')) {
	fargs[0][len - 2] = '\0';
    }
    safe_str(fargs[0], buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_v: Function form of %-substitution
 */

FUNCTION(fun_v)
{
	dbref aowner;
	int aflags, alen;
	char *sbuf, *sbufc, *tbuf, *str;
	ATTR *ap;

	tbuf = fargs[0];
	if (isalpha(tbuf[0]) && tbuf[1]) {

		/* Fetch an attribute from me.  First see if it exists, 
		 * returning a null string if it does not. 
		 */

		ap = atr_str(fargs[0]);
		if (!ap) {
			return;
		}
		/* If we can access it, return it, otherwise return a null
		 * string 
		 */
		tbuf = atr_pget(player, ap->number, &aowner, &aflags, &alen);
		if (See_attr(player, player, ap, aowner, aflags))
		    safe_known_str(tbuf, alen, buff, bufc);
		if (tbuf)
		    free_lbuf(tbuf);
		return;
	}
	/* Not an attribute, process as %<arg> */

	sbuf = alloc_sbuf("fun_v");
	sbufc = sbuf;
	safe_sb_chr('%', sbuf, &sbufc);
	safe_sb_str(fargs[0], sbuf, &sbufc);
	*sbufc = '\0';
	str = sbuf;
	exec(buff, bufc, 0, player, cause, EV_FIGNORE, &str, cargs, ncargs);
	free_sbuf(sbuf);
}

/* ---------------------------------------------------------------------------
 * fun_s: Force substitution to occur.
 */

FUNCTION(fun_s)
{
	char *str;

	str = fargs[0];
	exec(buff, bufc, 0, player, cause, EV_FIGNORE | EV_EVAL, &str,
	     cargs, ncargs);
}

/* ---------------------------------------------------------------------------
 * fun_con: Returns first item in contents list of object/room
 */

FUNCTION(fun_con)
{
	dbref it;

	it = match_thing(player, fargs[0]);

	if ((it != NOTHING) &&
	    (Has_contents(it)) &&
	    (Examinable(player, it) ||
	     (where_is(player) == it) ||
	     (it == cause))) {
		safe_dbref(buff, bufc, Contents(it));
		return;
	}
	safe_nothing(buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_exit: Returns first exit in exits list of room.
 */

FUNCTION(fun_exit)
{
	dbref it, exit;
	int key;

	it = match_thing(player, fargs[0]);
	if (Good_obj(it) && Has_exits(it) && Good_obj(Exits(it))) {
		key = 0;
		if (Examinable(player, it))
			key |= VE_LOC_XAM;
		if (Dark(it))
			key |= VE_LOC_DARK;
		DOLIST(exit, Exits(it)) {
			if (exit_visible(exit, player, key)) {
			    safe_dbref(buff, bufc, exit);
			}
		}
	}
	safe_nothing(buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_next: return next thing in contents or exits chain
 */

FUNCTION(fun_next)
{
	dbref it, loc, exit, ex_here;
	int key;

	it = match_thing(player, fargs[0]);
	if (Good_obj(it) && Has_siblings(it)) {
		loc = where_is(it);
		ex_here = Good_obj(loc) ? Examinable(player, loc) : 0;
		if (ex_here || (loc == player) || (loc == where_is(player))) {
			if (!isExit(it)) {
				safe_dbref(buff, bufc, Next(it));
				return;
			} else {
				key = 0;
				if (ex_here)
					key |= VE_LOC_XAM;
				if (Dark(loc))
					key |= VE_LOC_DARK;
				DOLIST(exit, it) {
					if ((exit != it) &&
					  exit_visible(exit, player, key)) {
						safe_dbref(buff, bufc, exit);
						return;
					}
				}
			}
		}
	}
	safe_nothing(buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_loc: Returns the location of something
 */

FUNCTION(fun_loc)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (locatable(player, it, cause)) {
		safe_dbref(buff, bufc, Location(it));
	} else {
		safe_nothing(buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_where: Returns the "true" location of something
 */

FUNCTION(fun_where)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (locatable(player, it, cause)) {
		safe_dbref(buff, bufc, where_is(it));
	} else {
		safe_nothing(buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_rloc: Returns the recursed location of something (specifying #levels)
 */

FUNCTION(fun_rloc)
{
	int i, levels;
	dbref it;

	levels = atoi(fargs[1]);
	if (levels > mudconf.ntfy_nest_lim)
		levels = mudconf.ntfy_nest_lim;

	it = match_thing(player, fargs[0]);
	if (locatable(player, it, cause)) {
		for (i = 0; i < levels; i++) {
			if (!Good_obj(it) || !Has_location(it))
				break;
			it = Location(it);
		}
		safe_dbref(buff, bufc, it);
		return;
	}
	safe_nothing(buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_room: Find the room an object is ultimately in.
 */

FUNCTION(fun_room)
{
	dbref it;
	int count;

	it = match_thing(player, fargs[0]);
	if (locatable(player, it, cause)) {
		for (count = mudconf.ntfy_nest_lim; count > 0; count--) {
			it = Location(it);
			if (!Good_obj(it))
				break;
			if (isRoom(it)) {
				safe_dbref(buff, bufc, it);
				return;
			}
		}
		safe_nothing(buff, bufc);
	} else if (isRoom(it)) {
		safe_dbref(buff, bufc, it);
	} else {
		safe_nothing(buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_owner: Return the owner of an object.
 */

FUNCTION(fun_owner)
{
	dbref it, aowner;
	int atr, aflags;

	if (parse_attrib(player, fargs[0], &it, &atr)) {
		if (atr == NOTHING) {
			it = NOTHING;
		} else {
			atr_pget_info(it, atr, &aowner, &aflags);
			it = aowner;
		}
	} else {
		it = match_thing(player, fargs[0]);
		if (it != NOTHING)
			it = Owner(it);
	}
	safe_dbref(buff, bufc, it);
}

/* ---------------------------------------------------------------------------
 * fun_controls: Does x control y?
 */

FUNCTION(fun_controls)
{
	dbref x, y;

	x = match_thing(player, fargs[0]);
	if (x == NOTHING) {
		safe_tprintf_str(buff, bufc, "%s", "#-1 ARG1 NOT FOUND");
		return;
	}
	y = match_thing(player, fargs[1]);
	if (y == NOTHING) {
		safe_tprintf_str(buff, bufc, "%s", "#-1 ARG2 NOT FOUND");
		return;
	}
	safe_ltos(buff, bufc, Controls(x, y));
}

/* ---------------------------------------------------------------------------
 * fun_sees: Can X see Y in the normal Contents list of the room. If X
 *           or Y do not exist, 0 is returned.
 */

FUNCTION(fun_sees)
{
    dbref it, thing;
    int can_see_loc;

    if ((it = match_thing(player, fargs[0])) == NOTHING) {
	safe_chr('0', buff, bufc);
	return;
    }

    thing = match_thing(player, fargs[1]);
    if (!Good_obj(thing)) {
	safe_chr('0', buff, bufc);
	return;
    }

    can_see_loc = (!Dark(Location(thing)) ||
		   (mudconf.see_own_dark &&
		    Examinable(player, Location(thing))));
    safe_ltos(buff, bufc, can_see(it, thing, can_see_loc));
}

/* ---------------------------------------------------------------------------
 * fun_fullname: Return the fullname of an object (good for exits)
 */

FUNCTION(fun_fullname)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (it == NOTHING) {
		return;
	}
	if (!mudconf.read_rem_name) {
		if (!nearby_or_control(player, it) &&
		    (!isPlayer(it))) {
			safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
			return;
		}
	}
	safe_name(it, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_name: Return the name of an object
 */

FUNCTION(fun_name)
{
	dbref it;
	char *s, *temp;

	it = match_thing(player, fargs[0]);
	if (it == NOTHING) {
		return;
	}
	if (!mudconf.read_rem_name) {
		if (!nearby_or_control(player, it) && !isPlayer(it) && !Long_Fingers(player)) {
			safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
			return;
		}
	}
	temp = *bufc;
	safe_name(it, buff, bufc);
	if (isExit(it)) {
		for (s = temp; (s != *bufc) && (*s != ';'); s++) ;
		if (*s == ';')
			*bufc = s;
	}
}

/* ---------------------------------------------------------------------------
 * fun_match, fun_strmatch: Match arg2 against each word of arg1 returning
 * index of first match, or against the whole string.
 */

FUNCTION(fun_match)
{
	int wcount;
	char *r, *s, sep;

	varargs_preamble("MATCH", 3);

	/* Check each word individually, returning the word number of the 
	 * first one that matches.  If none match, return 0. 
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], sep);
	do {
		r = split_token(&s, sep);
		if (quick_wild(fargs[1], r)) {
			safe_ltos(buff, bufc, wcount);
			return;
		}
		wcount++;
	} while (s);
	safe_chr('0', buff, bufc);
}

FUNCTION(fun_strmatch)
{
	/* Check if we match the whole string.  If so, return 1 */

	if (quick_wild(fargs[1], fargs[0])) {
		safe_chr('1', buff, bufc);
	} else {
		safe_chr('0', buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_streq: compare two strings case-insensitively.
 */

FUNCTION(fun_streq)
{
    if (!string_compare(fargs[0], fargs[1])) {
	safe_chr('1', buff, bufc);
    } else {
	safe_chr('0', buff, bufc);
    }
    return;
}

/* ---------------------------------------------------------------------------
 * fun_extract: extract words from string:
 * extract(foo bar baz,1,2) returns 'foo bar'
 * extract(foo bar baz,2,1) returns 'bar'
 * extract(foo bar baz,2,2) returns 'bar baz'
 * 
 * Now takes optional separator extract(foo-bar-baz,1,2,-) returns 'foo-bar'
 */

FUNCTION(fun_extract)
{
	int start, len;
	char *r, *s, *t, sep;

	varargs_preamble("EXTRACT", 4);

	s = fargs[0];
	start = atoi(fargs[1]);
	len = atoi(fargs[2]);

	if ((start < 1) || (len < 1)) {
		return;
	}
	/* Skip to the start of the string to save */

	start--;
	s = trim_space_sep(s, sep);
	while (start && s) {
		s = next_token(s, sep);
		start--;
	}

	/* If we ran of the end of the string, return nothing */

	if (!s || !*s) {
		return;
	}
	/* Count off the words in the string to save */

	r = s;
	len--;
	while (len && s) {
		s = next_token(s, sep);
		len--;
	}

	/* Chop off the rest of the string, if needed */

	if (s && *s)
		t = split_token(&s, sep);
	safe_str(r, buff, bufc);
}

int xlate(arg)
char *arg;
{
	int temp;
	char *temp2;

	if (arg[0] == '#') {
		arg++;
		if (is_integer(arg)) {
			temp = atoi(arg);
			if (temp == -1)
				temp = 0;
			return temp;
		}
		return 0;
	}
	temp2 = trim_space_sep(arg, ' ');
	if (!*temp2)
		return 0;
	if (is_integer(temp2))
		return atoi(temp2);
	return 1;
}

/* ---------------------------------------------------------------------------
 * fun_index:  like extract(), but it works with an arbitrary separator.
 * index(a b | c d e | f gh | ij k, |, 2, 1) => c d e
 * index(a b | c d e | f gh | ij k, |, 2, 2) => c d e | f g h
 */

FUNCTION(fun_index)
{
	int start, end;
	char c, *s, *p;

	s = fargs[0];
	c = *fargs[1];
	start = atoi(fargs[2]);
	end = atoi(fargs[3]);

	if ((start < 1) || (end < 1) || (*s == '\0'))
		return;
	if (c == '\0')
		c = ' ';

	/* move s to point to the start of the item we want */

	start--;
	while (start && s && *s) {
		if ((s = (char *)index(s, c)) != NULL)
			s++;
		start--;
	}

	/* skip over just spaces */

	while (s && (*s == ' '))
		s++;
	if (!s || !*s)
		return;

	/* figure out where to end the string */

	p = s;
	while (end && p && *p) {
		if ((p = (char *)index(p, c)) != NULL) {
			if (--end == 0) {
				do {
					p--;
				} while ((*p == ' ') && (p > s));
				*(++p) = '\0';
				safe_str(s, buff, bufc);
				return;
			} else {
				p++;
			}
		}
	}

	/* if we've gotten this far, we've run off the end of the string */

	safe_str(s, buff, bufc);
}


FUNCTION(fun_cat)
{
	int i;

	safe_str(fargs[0], buff, bufc);
	for (i = 1; i < nfargs; i++) {
		safe_chr(' ', buff, bufc);
		safe_str(fargs[i], buff, bufc);
	}
}

FUNCTION(fun_version)
{
	safe_str(mudstate.version, buff, bufc);
}
FUNCTION(fun_strlen)
{
	safe_ltos(buff, bufc, (int)strlen((char *)strip_ansi(fargs[0])));
}

FUNCTION(fun_num)
{
	safe_dbref(buff, bufc, match_thing(player, fargs[0]));
}

FUNCTION(fun_pmatch)
{
    char *name, *temp, *tp;
    dbref thing;
    dbref *p_ptr;

    /* If we have a valid dbref, it's okay if it's a player. */

    if ((*fargs[0] == NUMBER_TOKEN) && fargs[0][1]) {
	thing = parse_dbref(fargs[0] + 1);
	if (Good_obj(thing) && isPlayer(thing)) {
	    safe_dbref(buff, bufc, thing);
	} else {
	    safe_nothing(buff, bufc);
	}
	return;
    }

    /* If we have *name, just advance past the *; it doesn't matter */

    name = fargs[0];
    if (*fargs[0] == LOOKUP_TOKEN) {
	name++;
    }

    /* Look up the full name */

    tp = temp = alloc_lbuf("fun_pmatch");
    safe_str(name, temp, &tp);
    for (tp = temp; *tp; tp++)
	*tp = ToLower(*tp);
    p_ptr = (int *) hashfind(temp, &mudstate.player_htab);
    free_lbuf(temp);

    if (p_ptr) {
	/* We've got it. Check to make sure it's a good object. */
	if (Good_obj(*p_ptr) && isPlayer(*p_ptr)) {
	    safe_dbref(buff, bufc, (int) *p_ptr);
	} else {
	    safe_nothing(buff, bufc);
	}
	return;
    }

    /* We haven't found anything. Now we try a partial match. */

    thing = find_connected_ambiguous(player, name);
    if (thing == AMBIGUOUS) {
	safe_known_str("#-2", 3, buff, bufc);
    } else if (Good_obj(thing) && isPlayer(thing)) {
	safe_dbref(buff, bufc, thing);
    } else {
	safe_nothing(buff, bufc);
    }
}

FUNCTION(fun_pfind)
{
	dbref thing;

	if (*fargs[0] == '#') {
		safe_dbref(buff, bufc, match_thing(player, fargs[0]));
		return;
	}
	if (!((thing = lookup_player(player, fargs[0], 1)) == NOTHING)) {
		safe_dbref(buff, bufc, thing);
		return;
	} else
		safe_nomatch(buff, bufc);
}

/*-------------------------------------------------------------------------
 * Comparison functions.
 */

FUNCTION(fun_gt)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) > aton(fargs[1])));
}
FUNCTION(fun_gte)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) >= aton(fargs[1])));
}
FUNCTION(fun_lt)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) < aton(fargs[1])));
}
FUNCTION(fun_lte)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) <= aton(fargs[1])));
}
FUNCTION(fun_eq)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) == aton(fargs[1])));
}
FUNCTION(fun_neq)
{
	safe_ltos(buff, bufc, (aton(fargs[0]) != aton(fargs[1])));
}

FUNCTION(fun_and)
{
	int i;
	
	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		for (i = 0; (i < nfargs) && atoi(fargs[i]); i++) ;
		safe_ltos(buff, bufc, i == nfargs);
	}
}

FUNCTION(fun_or)
{
	int i;
	
	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		for (i = 0; (i < nfargs) && !atoi(fargs[i]); i++) ;
		safe_ltos(buff, bufc, i != nfargs);
	}
}

FUNCTION(fun_xor)
{
	int i, val;

	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		val = atoi(fargs[0]);
		for (i = 1; i < nfargs; i++) {
			if (val) {
				val = !atoi(fargs[i]);
			} else {
				val = atoi(fargs[i]);
			}
		}
		safe_ltos(buff, bufc, val ? 1 : 0);
	}
	return;
}

FUNCTION(fun_not)
{
	safe_ltos(buff, bufc, !atoi(fargs[0]));
}

/*-------------------------------------------------------------------------
 * List-based numeric functions.
 */

FUNCTION(fun_ladd)
{
    NVAL sum;
    char *cp, *curr, sep;

    varargs_preamble("LADD", 2);

    sum = 0;
    cp = trim_space_sep(fargs[0], sep);
    while (cp) {
	curr = split_token(&cp, sep);
	sum += aton(curr);
    }
    fval(buff, bufc, sum);
}

FUNCTION(fun_lor)
{
    int i;
    char *cp, *curr, sep;

    varargs_preamble("LOR", 2);

    i = 0;
    cp = trim_space_sep(fargs[0], sep);
    while (cp && !i) {
	curr = split_token(&cp, sep);
	i = atoi(curr);
    }
    safe_ltos(buff, bufc, (i != 0));
}

FUNCTION(fun_land)
{
    int i;
    char *cp, *curr, sep;

    varargs_preamble("LAND", 2);

    i = 1;
    cp = trim_space_sep(fargs[0], sep);
    while (cp && i) {
	curr = split_token(&cp, sep);
	i = atoi(curr);
    }
    safe_ltos(buff, bufc, (i != 0));
}

FUNCTION(fun_lorbool)
{
    int i;
    char *cp, *curr, sep;

    varargs_preamble("LORBOOL", 2);

    i = 0;
    cp = trim_space_sep(fargs[0], sep);
    while (cp && !i) {
	curr = split_token(&cp, sep);
	i = xlate(curr);
    }
    safe_ltos(buff, bufc, (i != 0));
}

FUNCTION(fun_landbool)
{
    int i;
    char *cp, *curr, sep;

    varargs_preamble("LANDBOOL", 2);

    i = 1;
    cp = trim_space_sep(fargs[0], sep);
    while (cp && i) {
	curr = split_token(&cp, sep);
	i = xlate(curr);
    }
    safe_ltos(buff, bufc, (i != 0));
}

FUNCTION(fun_lmax)
{
    NVAL max, val;
    char *cp, *curr, sep;

    varargs_preamble("LMAX", 2);

    cp = trim_space_sep(fargs[0], sep);
    if (cp) {
	curr = split_token(&cp, sep);
	max = aton(curr);
	while (cp) {
	    curr = split_token(&cp, sep);
	    val = aton(curr);
	    if (max < val)
		max = val;
	}
	fval(buff, bufc, max);
    }
}

FUNCTION(fun_lmin)
{
    NVAL min, val;
    char *cp, *curr, sep;

    varargs_preamble("LMIN", 2);

    cp = trim_space_sep(fargs[0], sep);
    if (cp) {
	curr = split_token(&cp, sep);
	min = aton(curr);
	while (cp) {
	    curr = split_token(&cp, sep);
	    val = aton(curr);
	    if (min > val)
		min = val;
	}
	fval(buff, bufc, min);
    }
}

/*-------------------------------------------------------------------------
 *  True boolean functions.
 */

FUNCTION(fun_andbool)
{
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	for (i = 0; (i < nfargs) && xlate(fargs[i]); i++)
	    ;
	safe_ltos(buff, bufc, i == nfargs);
    }
    return;
}

FUNCTION(fun_orbool)
{
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	for (i = 0; (i < nfargs) && !xlate(fargs[i]); i++)
	    ;
	safe_ltos(buff, bufc, i != nfargs);
    }
    return;
}

FUNCTION(fun_xorbool)
{
    int i, val;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	val = xlate(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    if (val) {
		val = !xlate(fargs[i]);
	    } else {
		val = xlate(fargs[i]);
	    }
	}
	safe_ltos(buff, bufc, val ? 1 : 0);
    }
    return;
}

FUNCTION(fun_notbool)
{
	safe_ltos(buff, bufc, !xlate(fargs[0]));
}

FUNCTION(fun_t)
{
	safe_ltos(buff, bufc, xlate(fargs[0]) ? 1 : 0);
}

FUNCTION(fun_sqrt)
{
	double val;

	val = atof(fargs[0]);
	if (val < 0) {
		safe_str("#-1 SQUARE ROOT OF NEGATIVE", buff, bufc);
	} else if (val == 0) {
		safe_chr('0', buff, bufc);
	} else {
		fval(buff, bufc, sqrt(val));
	}
}

FUNCTION(fun_add)
{
	NVAL sum;
	int i;

	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		sum = aton(fargs[0]);
		for (i = 1; i < nfargs; i++) {
			sum += aton(fargs[i]);
		}
		fval(buff, bufc, sum);
	}
	return;
}

FUNCTION(fun_sub)
{
	fval(buff, bufc, aton(fargs[0]) - aton(fargs[1]));
}

FUNCTION(fun_mul)
{
    NVAL prod;
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	prod = aton(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    prod *= aton(fargs[i]);
	}
	fval(buff, bufc, prod);
    }
    return;
}

FUNCTION(fun_floor)
{
	safe_tprintf_str(buff, bufc, "%.0f", floor(atof(fargs[0])));
}
FUNCTION(fun_ceil)
{
	safe_tprintf_str(buff, bufc, "%.0f", ceil(atof(fargs[0])));
}
FUNCTION(fun_round)
{
	const char *fstr;
	char *oldp;
	
	oldp = *bufc;
	
	switch (atoi(fargs[1])) {
	case 1:
		fstr = "%.1f";
		break;
	case 2:
		fstr = "%.2f";
		break;
	case 3:
		fstr = "%.3f";
		break;
	case 4:
		fstr = "%.4f";
		break;
	case 5:
		fstr = "%.5f";
		break;
	case 6:
		fstr = "%.6f";
		break;
	default:
		fstr = "%.0f";
		break;
	}
	safe_tprintf_str(buff, bufc, (char *)fstr, atof(fargs[0]));

	/* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

	if (!strcmp(oldp, "-0")) {
		*oldp = '0';
		*bufc = oldp + 1;
	}
}

FUNCTION(fun_trunc)
{
    safe_ltos(buff, bufc, atoi(fargs[0]));
}

FUNCTION(fun_div)
{
	int bot;

	bot = atoi(fargs[1]);
	if (bot == 0) {
		safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
	} else {
		safe_ltos(buff, bufc, (atoi(fargs[0]) / bot));
	}
}

FUNCTION(fun_fdiv)
{
	double bot;

	bot = atof(fargs[1]);
	if (bot == 0) {
		safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
	} else {
		fval(buff, bufc, (atof(fargs[0]) / bot));
	}
}

FUNCTION(fun_mod)
{
	int bot;

	bot = atoi(fargs[1]);
	if (bot == 0)
		bot = 1;
	safe_ltos(buff, bufc, atoi(fargs[0]) % bot);
}

FUNCTION(fun_pi)
{
	safe_known_str("3.141592654", 11, buff, bufc);
}
FUNCTION(fun_e)
{
	safe_known_str("2.718281828", 11, buff, bufc);
}

FUNCTION(fun_sin)
{
	fval(buff, bufc, sin(atof(fargs[0])));
}
FUNCTION(fun_cos)
{
	fval(buff, bufc, cos(atof(fargs[0])));
}
FUNCTION(fun_tan)
{
	fval(buff, bufc, tan(atof(fargs[0])));
}

FUNCTION(fun_exp)
{
	fval(buff, bufc, exp(atof(fargs[0])));
}

FUNCTION(fun_power)
{
	double val1, val2;

	val1 = atof(fargs[0]);
	val2 = atof(fargs[1]);
	if (val1 < 0) {
		safe_str("#-1 POWER OF NEGATIVE", buff, bufc);
	} else {
		fval(buff, bufc, pow(val1, val2));
	}
}

FUNCTION(fun_ln)
{
	double val;

	val = atof(fargs[0]);
	if (val > 0)
		fval(buff, bufc, log(val));
	else
		safe_str("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
}

FUNCTION(fun_log)
{
	double val, base;

	if (!fn_range_check("LOG", nfargs, 1, 2, buff, bufc))
	    return;

	val = atof(fargs[0]);

	if (nfargs == 2)
	    base = atof(fargs[1]);
	else
	    base = 10;

	if ((val <= 0) || (base <= 0))
	    safe_str("#-1 LOG OF NEGATIVE OR ZERO", buff, bufc);
	else if (base == 1)
	    safe_str("#-1 DIVISION BY ZERO", buff, bufc);
	else
	    fval(buff, bufc, log(val) / log(base));
}


FUNCTION(fun_asin)
{
	double val;

	val = atof(fargs[0]);
	if ((val < -1) || (val > 1)) {
		safe_str("#-1 ASIN ARGUMENT OUT OF RANGE", buff, bufc);
	} else {
		fval(buff, bufc, asin(val));
	}
}

FUNCTION(fun_acos)
{
	double val;

	val = atof(fargs[0]);
	if ((val < -1) || (val > 1)) {
		safe_str("#-1 ACOS ARGUMENT OUT OF RANGE", buff, bufc);
	} else {
		fval(buff, bufc, acos(val));
	}
}

FUNCTION(fun_atan)
{
	fval(buff, bufc, atan(atof(fargs[0])));
}

FUNCTION(fun_dist2d)
{
	int d;
	double r;

	d = atoi(fargs[0]) - atoi(fargs[2]);
	r = (double)(d * d);
	d = atoi(fargs[1]) - atoi(fargs[3]);
	r += (double)(d * d);
	d = (int)(sqrt(r) + 0.5);
	safe_ltos(buff, bufc, d);
}

FUNCTION(fun_dist3d)
{
	int d;
	double r;

	d = atoi(fargs[0]) - atoi(fargs[3]);
	r = (double)(d * d);
	d = atoi(fargs[1]) - atoi(fargs[4]);
	r += (double)(d * d);
	d = atoi(fargs[2]) - atoi(fargs[5]);
	r += (double)(d * d);
	d = (int)(sqrt(r) + 0.5);
	safe_ltos(buff, bufc, d);
}

/* ------------------------------------------------------------------------
 * Vector functions: VADD, VSUB, VMUL, VCROSS, VMAG, VUNIT, VDIM
 * Vectors are space-separated numbers.
 */

#define MAXDIM 20
#define VADD_F 0
#define VSUB_F 1
#define VMUL_F 2
#define VDOT_F 3
#define VCROSS_F 4

static void handle_vectors(vecarg1, vecarg2, buff, bufc, sep, osep, flag)
    char *vecarg1;
    char *vecarg2;
    char *buff;
    char **bufc;
    char sep;
    char osep;
    int flag;
{
    char *v1[LBUF_SIZE], *v2[LBUF_SIZE];
    char vres[MAXDIM][LBUF_SIZE];
    double scalar;
    int n, m, i;

    /*
     * split the list up, or return if the list is empty 
     */
    if (!vecarg1 || !*vecarg1 || !vecarg2 || !*vecarg2) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, vecarg1, sep);
    m = list2arr(v2, LBUF_SIZE, vecarg2, sep);

    /* It's okay to have vmul() be passed a scalar first or second arg,
     * but everything else has to be same-dimensional.
     */
    if ((n != m) &&
	!((flag == VMUL_F) && ((n == 1) || (m == 1)))) {
	safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc);
	return;
    }
    if (n > MAXDIM) {
	safe_str("#-1 TOO MANY DIMENSIONS ON VECTORS", buff, bufc);
	return;
    }

    switch (flag) {
	case VADD_F:
	    for (i = 0; i < n; i++) {
		fval_buf(vres[i], atof(v1[i]) + atof(v2[i]));
		v1[i] = (char *) vres[i];
	    }
	    arr2list(v1, n, buff, bufc, osep);
	    return;
	    /* NOTREACHED */
	case VSUB_F:
	    for (i = 0; i < n; i++) {
		fval_buf(vres[i], atof(v1[i]) - atof(v2[i]));
		v1[i] = (char *) vres[i];
	    }
	    arr2list(v1, n, buff, bufc, osep);
	    return;
	    /* NOTREACHED */
	case VMUL_F:
	    /* if n or m is 1, this is scalar multiplication.
	     * otherwise, multiply elementwise.
	     */
	    if (n == 1) {
		scalar = atof(v1[0]);
		for (i = 0; i < m; i++) {
		    fval_buf(vres[i], atof(v2[i]) * scalar);
		    v1[i] = (char *) vres[i];
		}
		n = m;
	    } else if (m == 1) {
		scalar = atof(v2[0]);
		for (i = 0; i < n; i++) {
		    fval_buf(vres[i], atof(v1[i]) * scalar);
		    v1[i] = (char *) vres[i];
		}
	    } else {
		/* vector elementwise product.
		 *
		 * Note this is a departure from TinyMUX, but an imitation
		 * of the PennMUSH behavior: the documentation in Penn
		 * claims it's a dot product, but the actual behavior
		 * isn't. We implement dot product separately!
		 */
		for (i = 0; i < n; i++) {
		    fval_buf(vres[i], atof(v1[i]) * atof(v2[i]));
		    v1[i] = (char *) vres[i];
		}
	    }
	    arr2list(v1, n, buff, bufc, osep);
	    return;
	    /* NOTREACHED */
	case VDOT_F:
	    scalar = 0;
	    for (i = 0; i < n; i++) {
		scalar += atof(v1[i]) * atof(v2[i]);
		v1[i] = (char *) vres[i];
	    }
	    fval(buff, bufc, scalar);
	    return;
	    /* NOTREACHED */
	default:
	    /* If we reached this, we're in trouble. */
	    safe_str("#-1 UNIMPLEMENTED", buff, bufc);
    }
}

FUNCTION(fun_vadd)
{
    char sep, osep;

    svarargs_preamble("VADD", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc, sep, osep, VADD_F);
}

FUNCTION(fun_vsub)
{
    char sep, osep;

    svarargs_preamble("VSUB", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc, sep, osep, VSUB_F);
}

FUNCTION(fun_vmul)
{
    char sep, osep;

    svarargs_preamble("VMUL", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc, sep, osep, VMUL_F);
}

FUNCTION(fun_vdot)
{
    /* dot product: (a,b,c) . (d,e,f) = ad + be + cf
     * 
     * no cross product implementation yet: it would be
     * (a,b,c) x (d,e,f) = (bf - ce, cd - af, ae - bd)
     */

    char sep, osep;

    svarargs_preamble("VDOT", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc, sep, osep, VDOT_F);
}

FUNCTION(fun_vmag)
{
    char *v1[LBUF_SIZE];
    int n, i;
    double tmp, res = 0;
    char sep;

    varargs_preamble("VMAG", 2);

    /*
     * split the list up, or return if the list is empty 
     */
    if (!fargs[0] || !*fargs[0]) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, fargs[0], sep);

    if (n > MAXDIM) {
	safe_str("#-1 TOO MANY DIMENSIONS ON VECTORS", buff, bufc);
	return;
    }
    /*
     * calculate the magnitude 
     */
    for (i = 0; i < n; i++) {
	tmp = atof(v1[i]);
	res += tmp * tmp;
    }

    if (res > 0) {
	fval(buff, bufc, sqrt(res));
    } else {
	safe_chr('0', buff, bufc);
    }
}

FUNCTION(fun_vunit)
{
    char *v1[LBUF_SIZE];
    char vres[MAXDIM][LBUF_SIZE];
    int n, i;
    double tmp, res = 0;
    char sep;

    varargs_preamble("VUNIT", 2);

    /*
     * split the list up, or return if the list is empty 
     */
    if (!fargs[0] || !*fargs[0]) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, fargs[0], sep);

    if (n > MAXDIM) {
	safe_str("#-1 TOO MANY DIMENSIONS ON VECTORS", buff, bufc);
	return;
    }
    /*
     * calculate the magnitude 
     */
    for (i = 0; i < n; i++) {
	tmp = atof(v1[i]);
	res += tmp * tmp;
    }

    if (res <= 0) {
	safe_str("#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR",
		 buff, bufc);
	return;
    }
    for (i = 0; i < n; i++) {
	fval_buf(vres[i], atof(v1[i]) / sqrt(res));
	v1[i] = (char *) vres[i];
    }

    arr2list(v1, n, buff, bufc, sep);
}

FUNCTION(fun_vdim)
{
    char sep;

    if (fargs == 0) {
	safe_chr('0', buff, bufc);
    } else {
	varargs_preamble("VDIM", 2);
	safe_ltos(buff, bufc, countwords(fargs[0], sep));
    }
}

/* ---------------------------------------------------------------------------
 * fun_comp: string compare.
 */

FUNCTION(fun_comp)
{
	int x;

	x = strcmp(fargs[0], fargs[1]);
	if (x > 0) {
		safe_chr('1', buff, bufc);
	} else if (x < 0) {
		safe_str("-1", buff, bufc);
	} else {
		safe_chr('0', buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_ncomp: numerical compare.
 */

FUNCTION(fun_ncomp)
{
    int x, y;

    x = atoi(fargs[0]);
    y = atoi(fargs[1]);

    if (x == y) {
	safe_chr('0', buff, bufc);
    } else if (x < y) {
	safe_str("-1", buff, bufc);
    } else {
	safe_chr('1', buff, bufc);
    }
}

/* ---------------------------------------------------------------------------
 * fun_xcon: Return a partial list of contents of an object, starting from
 *           a specified element in the list and copying a specified number
 *           of elements.
 */

FUNCTION(fun_xcon)
{
    dbref thing, it;
    char *bb_p;
    int i, first, last;

    it = match_thing(player, fargs[0]);

    bb_p = *bufc;
    if ((it != NOTHING) && (Has_contents(it)) &&
	(Examinable(player, it) || (Location(player) == it) ||
	 (it == cause))) {
	first = atoi(fargs[1]);
	last = atoi(fargs[2]);
	if ((first > 0) && (last > 0)) {

	    /* Move to the first object that we want */
	    for (thing = Contents(it), i = 1;
		 (i < first) && (thing != NOTHING) && (Next(thing) != thing);
		 thing = Next(thing), i++)
		;

	    /* Grab objects until we reach the last one we want */
	    for (i = 0;
		 (i < last) && (thing != NOTHING) && (Next(thing) != thing);
		 thing = Next(thing), i++) {
		if (*bufc != bb_p)
		    safe_chr(' ', buff, bufc);
		safe_dbref(buff, bufc, thing);
	    }
	}
    } else
	safe_nothing(buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_lcon: Return a list of contents.
 */

FUNCTION(fun_lcon)
{
	dbref thing, it;
	char *bb_p;

	it = match_thing(player, fargs[0]);
	bb_p = *bufc;
	if ((it != NOTHING) &&
	    (Has_contents(it)) &&
	    (Examinable(player, it) ||
	     (Location(player) == it) ||
	     (it == cause))) {
		DOLIST(thing, Contents(it)) {
		    if (*bufc != bb_p)
			safe_chr(' ', buff, bufc);
		    safe_dbref(buff, bufc, thing);
		}
	} else
		safe_nothing(buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_lexits: Return a list of exits.
 */

FUNCTION(fun_lexits)
{
	dbref thing, it, parent;
	char *bb_p;
	int exam, lev, key;
	int first = 1;
	
	it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Has_exits(it)) {
		safe_nothing(buff, bufc);
		return;
	}
	exam = Examinable(player, it);
	if (!exam && (where_is(player) != it) && (it != cause)) {
		safe_nothing(buff, bufc);
		return;
	}

	/* Return info for all parent levels */

	bb_p = *bufc;
	ITER_PARENTS(it, parent, lev) {

		/* Look for exits at each level */

		if (!Has_exits(parent))
			continue;
		key = 0;
		if (Examinable(player, parent))
			key |= VE_LOC_XAM;
		if (Dark(parent))
			key |= VE_LOC_DARK;
		if (Dark(it))
			key |= VE_BASE_DARK;
		DOLIST(thing, Exits(parent)) {
			if (exit_visible(thing, player, key)) {
			    if (*bufc != bb_p)
				safe_chr(' ', buff, bufc);
			    safe_dbref(buff, bufc, thing);
			}
		}
	}
	return;
}

/* --------------------------------------------------------------------------
 * fun_home: Return an object's home 
 */

FUNCTION(fun_home)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (!Good_obj(it) || !Examinable(player, it)) {
		safe_nothing(buff, bufc);
	} else if (Has_home(it)) {
		safe_dbref(buff, bufc, Home(it));
	} else if (Has_dropto(it)) {
		safe_dbref(buff, bufc, Dropto(it));
	} else if (isExit(it)) {
		safe_dbref(buff, bufc, where_is(it));
	} else {
		safe_nothing(buff, bufc);
	}
	return;
}

/* ---------------------------------------------------------------------------
 * fun_money: Return an object's value
 */

FUNCTION(fun_money)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if ((it == NOTHING) || !Examinable(player, it))
		safe_nothing(buff, bufc);
	else
		safe_ltos(buff, bufc, Pennies(it));
}

/* ---------------------------------------------------------------------------
 * fun_pos: Find a word in a string 
 */

FUNCTION(fun_pos)
{
	int i = 1;
	char *s, *t, *u;

	i = 1;
	s = strip_ansi(fargs[1]);
	while (*s) {
		u = s;
		t = fargs[0];
		while (*t && *t == *u)
			++t, ++u;
		if (*t == '\0') {
			safe_ltos(buff, bufc, i);
			return;
		}
		++i, ++s;
	}
	safe_nothing(buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_lpos: Find all occurrences of a character in a string, and return
 * a space-separated list of the positions, starting at 0. i.e.,
 * lpos(a-bc-def-g,-) ==> 1 4 8
 */

FUNCTION(fun_lpos)
{
    char *s, *bb_p;
    char c, tbuf[8];
    int i;

    if (!fargs[0] || !*fargs[0])
	return;

    c = (char) *(fargs[1]);
    if (!c)
	c = ' ';

    bb_p = *bufc;

    for (i = 0, s = strip_ansi(fargs[0]); *s; i++, s++) {
	if (*s == c) {
	    if (*bufc != bb_p) {
		safe_chr(' ', buff, bufc);
	    }
	    ltos(tbuf, i);
	    safe_str(tbuf, buff, bufc);
	}
    }

}

/* ---------------------------------------------------------------------------
 * ldelete: Remove a word from a string by place
 *  ldelete(<list>,<position>[,<separator>])
 *
 * insert: insert a word into a string by place
 *  insert(<list>,<position>,<new item> [,<separator>])
 *
 * replace: replace a word into a string by place
 *  replace(<list>,<position>,<new item>[,<separator>])
 */

#define	IF_DELETE	0
#define	IF_REPLACE	1
#define	IF_INSERT	2

static void do_itemfuns(buff, bufc, str, el, word, sep, flag)
char *buff, **bufc, *str, *word, sep;
int el, flag;
{
	int ct, overrun;
	char *sptr, *iptr, *eptr;
	char nullb;

	/* If passed a null string return an empty string, except that we
	 * are allowed to append to a null string. 
	 */

	if ((!str || !*str) && ((flag != IF_INSERT) || (el != 1))) {
		return;
	}
	/* we can't fiddle with anything before the first position */

	if (el < 1) {
		safe_str(str, buff, bufc);
		return;
	}
	/* Split the list up into 'before', 'target', and 'after' chunks
	 * pointed to by sptr, iptr, and eptr respectively. 
	 */

	nullb = '\0';
	if (el == 1) {
		/* No 'before' portion, just split off element 1 */

		sptr = NULL;
		if (!str || !*str) {
			eptr = NULL;
			iptr = NULL;
		} else {
			eptr = trim_space_sep(str, sep);
			iptr = split_token(&eptr, sep);
		}
	} else {
		/* Break off 'before' portion */

		sptr = eptr = trim_space_sep(str, sep);
		overrun = 1;
		for (ct = el; ct > 2 && eptr; eptr = next_token(eptr, sep), ct--) ;
		if (eptr) {
			overrun = 0;
			iptr = split_token(&eptr, sep);
		}
		/* If we didn't make it to the target element, just return
		 * the string.  Insert is allowed to continue if we are 
		 * exactly at the end of the string, but replace
		 * and delete are not. 
		 */

		if (!(eptr || ((flag == IF_INSERT) && !overrun))) {
			safe_str(str, buff, bufc);
			return;
		}
		/* Split the 'target' word from the 'after' portion. */

		if (eptr)
			iptr = split_token(&eptr, sep);
		else
			iptr = NULL;
	}

	switch (flag) {
	case IF_DELETE:	/* deletion */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			if (eptr)
				safe_chr(sep, buff, bufc);
		}
		if (eptr) {
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_REPLACE:	/* replacing */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			safe_chr(sep, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (eptr) {
			safe_chr(sep, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_INSERT:	/* insertion */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			safe_chr(sep, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (iptr) {
			safe_chr(sep, buff, bufc);
			safe_str(iptr, buff, bufc);
		}
		if (eptr) {
			safe_chr(sep, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	}
}

FUNCTION(fun_ldelete)
{				/* delete a word at position X of a list */
	char sep;

	varargs_preamble("LDELETE", 3);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), NULL, sep, IF_DELETE);
}

FUNCTION(fun_replace)
{				/* replace a word at position X of a list */
	char sep;

	varargs_preamble("REPLACE", 4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], sep, IF_REPLACE);
}

FUNCTION(fun_insert)
{				/* insert a word at position X of a list */
	char sep;

	varargs_preamble("INSERT", 4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], sep, IF_INSERT);
}

/* ---------------------------------------------------------------------------
 * fun_remove: Remove a word from a string
 */

FUNCTION(fun_remove)
{
	char *s, *sp, *word;
	char sep;
	int first, found;

	varargs_preamble("REMOVE", 3);
	if (index(fargs[1], sep)) {
		safe_str("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufc);
		return;
	}
	s = fargs[0];
	word = fargs[1];

	/* Walk through the string copying words until (if ever) we get to
	 * one that matches the target word. 
	 */

	sp = s;
	found = 0;
	first = 1;
	while (s) {
		sp = split_token(&s, sep);
		if (found || strcmp(sp, word)) {
			if (!first)
				safe_chr(sep, buff, bufc);
			safe_str(sp, buff, bufc);
			first = 0;
		} else {
			found = 1;
		}
	}
}

/* ---------------------------------------------------------------------------
 * fun_member: Is a word in a string
 */

FUNCTION(fun_member)
{
	int wcount;
	char *r, *s, sep;

	varargs_preamble("MEMBER", 3);
	wcount = 1;
	s = trim_space_sep(fargs[0], sep);
	do {
		r = split_token(&s, sep);
		if (!strcmp(fargs[1], r)) {
			safe_ltos(buff, bufc, wcount);
			return;
		}
		wcount++;
	} while (s);
	safe_chr('0', buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_secure, fun_escape: escape [, ], %, \, and the beginning of the string.
 */

FUNCTION(fun_secure)
{
	char *s;

	s = fargs[0];
	while (*s) {
		switch (*s) {
		case '%':
		case '$':
		case '\\':
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
		case ',':
		case ';':
			safe_chr(' ', buff, bufc);
			break;
		default:
			safe_chr(*s, buff, bufc);
		}
		s++;
	}
}

FUNCTION(fun_escape)
{
	char *s, *d;

	d = *bufc;
	s = fargs[0];
	while (*s) {
		switch (*s) {
		case '%':
		case '\\':
		case '[':
		case ']':
		case '{':
		case '}':
		case ';':
			safe_chr('\\', buff, bufc);
		default:
			if (*bufc == d)
				safe_chr('\\', buff, bufc);
			safe_chr(*s, buff, bufc);
		}
		s++;
	}
}

/* Take a character position and return which word that char is in.
 * wordpos(<string>, <charpos>)
 */
FUNCTION(fun_wordpos)
{
	int charpos, i;
	char *cp, *tp, *xp, sep;

	varargs_preamble("WORDPOS", 3);

	charpos = atoi(fargs[1]);
	cp = fargs[0];
	if ((charpos > 0) && (charpos <= strlen(cp))) {
		tp = &(cp[charpos - 1]);
		cp = trim_space_sep(cp, sep);
		xp = split_token(&cp, sep);
		for (i = 1; xp; i++) {
			if (tp < (xp + strlen(xp)))
				break;
			xp = split_token(&cp, sep);
		}
		safe_ltos(buff, bufc, i);
		return;
	}
	safe_nothing(buff, bufc);
	return;
}

FUNCTION(fun_type)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
		safe_str("#-1 NOT FOUND", buff, bufc);
		return;
	}
	switch (Typeof(it)) {
	case TYPE_ROOM:
		safe_known_str("ROOM", 4, buff, bufc);
		break;
	case TYPE_EXIT:
		safe_known_str("EXIT", 4, buff, bufc);
		break;
	case TYPE_PLAYER:
		safe_known_str("PLAYER", 6, buff, bufc);
		break;
	case TYPE_THING:
		safe_known_str("THING", 5, buff, bufc);
		break;
	default:
		safe_str("#-1 ILLEGAL TYPE", buff, bufc);
	}
	return;
}

/*---------------------------------------------------------------------------
 * fun_hasflag:  plus auxiliary function atr_has_flag.
 */

static int 
atr_has_flag(player, thing, attr, aowner, aflags, flagname)
    dbref player, thing;
    ATTR *attr;
    int aowner, aflags;
    char *flagname;
{
    if (!See_attr(player, thing, attr, aowner, aflags))
	return 0;
    else {
	if (string_prefix("dark", flagname))
	    return (aflags & AF_DARK);
	else if (string_prefix("wizard", flagname))
	    return (aflags & AF_WIZARD);
	else if (string_prefix("hidden", flagname))
	    return (aflags & AF_MDARK);
	else if (string_prefix("html", flagname))
	    return (aflags & AF_HTML);
	else if (string_prefix("locked", flagname))
	    return (aflags & AF_LOCK);
	else if (string_prefix("no_command", flagname))
	    return (aflags & AF_NOPROG);
	else if (string_prefix("no_parse", flagname))
	    return (aflags & AF_NOPARSE);
	else if (string_prefix("regexp", flagname))
	    return (aflags & AF_REGEXP);
	else if (string_prefix("god", flagname))
	    return (aflags & AF_GOD);
	else if (string_prefix("visual", flagname))
	    return (aflags & AF_VISUAL);
	else if (string_prefix("no_inherit", flagname))
	    return (aflags & AF_PRIVATE);
	else
	    return 0;
    }
}

FUNCTION(fun_hasflag)
{
    dbref it, aowner;
    int atr, aflags;
    ATTR *ap;

    if (parse_attrib(player, fargs[0], &it, &atr)) {
	if (atr == NOTHING) {
	    safe_str("#-1 NOT FOUND", buff, bufc);
	} else {
	    ap = atr_num(atr);
	    atr_pget_info(it, atr, &aowner, &aflags);
	    if (atr_has_flag(player, it, ap, aowner, aflags, fargs[1])) {
		safe_chr('1', buff, bufc);
	    } else {
		safe_chr('0', buff, bufc);
	    }
	}
    } else {
	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
	    safe_str("#-1 NOT FOUND", buff, bufc);
	    return;
	}
	if (mudconf.pub_flags || Examinable(player, it) || (it == cause)) {
	    if (has_flag(player, it, fargs[1])) {
		safe_chr('1', buff, bufc);
	    } else {
		safe_chr('0', buff, bufc);
            }
	} else {
	    safe_noperm(buff, bufc);
	}
    }
}

FUNCTION(fun_haspower)
{
	dbref it;

	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
		safe_str("#-1 NOT FOUND", buff, bufc);
		return;
	}
	if (mudconf.pub_flags || Examinable(player, it) || (it == cause)) {
		if (has_power(player, it, fargs[1])) {
			safe_chr('1', buff, bufc);
		} else {
			safe_chr('0', buff, bufc);
		}
	} else {
		safe_noperm(buff, bufc);
	}
}

FUNCTION(fun_delete)
{
    char *s, *savep;
    int count, start, nchars, len, have_normal;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);
    len = strlen(strip_ansi(s));

    if ((start >= len) || (nchars <= 0)) {
	safe_str(s, buff, bufc);
	return;
    }

    have_normal = 1;
    for (count = 0; *s && (count < len); ) {
	if (*s == ESC_CHAR) {
	    /* Start of an ANSI code. Skip to the end. */
	    savep = s;
	    while (*s && (*s != ANSI_END)) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (*s) {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    if (!strncmp(savep, ANSI_NORMAL, 4))
		have_normal = 1;
	    else
		have_normal = 0;
	} else {
	    if ((count >= start) && (count < start + nchars)) {
		s++;
	    } else {
		safe_chr(*s, buff, bufc);
		s++;
	    }
	    count++;
	}
    }

    if (!have_normal)
	safe_ansi_normal(buff, bufc);
}

FUNCTION(fun_lock)
{
	dbref it, aowner;
	int aflags, alen;
	char *tbuf;
	ATTR *attr;
	struct boolexp *bool;

	/* Parse the argument into obj + lock */

	if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
		return;

	/* Get the attribute and decode it if we can read it */

	tbuf = atr_get(it, attr->number, &aowner, &aflags, &alen);
	if (Read_attr(player, it, attr, aowner, aflags)) {
		bool = parse_boolexp(player, tbuf, 1);
		free_lbuf(tbuf);
		tbuf = (char *)unparse_boolexp_function(player, bool);
		free_boolexp(bool);
		safe_str(tbuf, buff, bufc);
	} else
		free_lbuf(tbuf);
}

FUNCTION(fun_elock)
{
	dbref it, victim, aowner;
	int aflags, alen;
	char *tbuf;
	ATTR *attr;
	struct boolexp *bool;

	/* Parse lock supplier into obj + lock */

	if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
		return;

	/* Get the victim and ensure we can do it */

	victim = match_thing(player, fargs[1]);
	if (!Good_obj(victim)) {
		safe_str("#-1 NOT FOUND", buff, bufc);
	} else if (!nearby_or_control(player, victim) &&
		   !nearby_or_control(player, it)) {
		safe_str("#-1 TOO FAR AWAY", buff, bufc);
	} else {
		tbuf = atr_get(it, attr->number, &aowner, &aflags, &alen);
		if ((aflags & AF_IS_LOCK) || 
		    Read_attr(player, it, attr, aowner, aflags)) {
		    if (Pass_Locks(player)) {
			safe_chr('1', buff, bufc);
		    } else {
			bool = parse_boolexp(player, tbuf, 1);
			safe_ltos(buff, bufc, eval_boolexp(victim, it, it,
							   bool));
			free_boolexp(bool);
		    }
		} else {
		    safe_chr('0', buff, bufc);
		}
		free_lbuf(tbuf);
	}
}

/* ---------------------------------------------------------------------------
 * fun_lwho: Return list of connected users.
 */

FUNCTION(fun_lwho)
{
	make_ulist(player, buff, bufc);
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
 * fun_nearby: Return whether or not obj1 is near obj2.
 */

FUNCTION(fun_nearby)
{
	dbref obj1, obj2;

	obj1 = match_thing(player, fargs[0]);
	obj2 = match_thing(player, fargs[1]);
	if (!(nearby_or_control(player, obj1) ||
	      nearby_or_control(player, obj2))) {
		safe_chr('0', buff, bufc);
	} else if (nearby(obj1, obj2)) {
		safe_chr('1', buff, bufc);
	} else {
		safe_chr('0', buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_obj, fun_poss, and fun_subj: perform pronoun sub for object.
 */

static void process_sex(player, what, token, buff, bufc)
dbref player;
char *what, *buff, **bufc;
const char *token;
{
	dbref it;
	char *str;

	it = match_thing(player, what);
	if (!Good_obj(it) ||
	    (!isPlayer(it) && !nearby_or_control(player, it))) {
		safe_nomatch(buff, bufc);
	} else {
		str = (char *)token;
		exec(buff, bufc, 0, it, it, 0, &str, (char **)NULL, 0);
	}
}

FUNCTION(fun_obj)
{
	process_sex(player, fargs[0], "%o", buff, bufc);
}

FUNCTION(fun_poss)
{
	process_sex(player, fargs[0], "%p", buff, bufc);
}

FUNCTION(fun_subj)
{
	process_sex(player, fargs[0], "%s", buff, bufc);
}

FUNCTION(fun_aposs)
{
	process_sex(player, fargs[0], "%a", buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_mudname: Return the name of the mud.
 */

FUNCTION(fun_mudname)
{
	safe_str(mudconf.mud_name, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

FUNCTION(fun_lcstr)
{
	char *ap;

	ap = fargs[0];
	while (*ap && ((*bufc - buff) < LBUF_SIZE - 1)) {
		**bufc = ToLower(*ap);
		ap++;
		(*bufc)++;
	}
}

FUNCTION(fun_ucstr)
{
	char *ap;

	ap = fargs[0];
	while (*ap && ((*bufc - buff) < LBUF_SIZE - 1)) {
		**bufc = ToUpper(*ap);
		ap++;
		(*bufc)++;
	}
}

FUNCTION(fun_capstr)
{
	char *s;

	s = *bufc;

	safe_str(fargs[0], buff, bufc);
	*s = ToUpper(*s);
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
 * fun_lattr: Return list of attributes I can see on the object.
 */

FUNCTION(fun_lattr)
{
	dbref thing;
	int ca, first;
	ATTR *attr;

	/* Check for wildcard matching.  parse_attrib_wild checks for read
	 * permission, so we don't have to.  Have p_a_w assume the
	 * slash-star if it is missing. 
	 */

	first = 1;
	olist_push();
	if (parse_attrib_wild(player, fargs[0], &thing, 0, 0, 1)) {
		for (ca = olist_first(); ca != NOTHING; ca = olist_next()) {
			attr = atr_num(ca);
			if (attr) {
				if (!first)
					safe_chr(' ', buff, bufc);
				first = 0;
				safe_str((char *)attr->name, buff, bufc);
			}
		}
	} else {
	        if (! mudconf.lattr_oldstyle)
		        safe_nomatch(buff, bufc);
	}
	olist_pop();
	return;
}

/* ---------------------------------------------------------------------------
 * do_reverse, fun_reverse, fun_revwords: Reverse things.
 */

static void do_reverse(from, to)
    char *from, *to;
{
    char *tp;

    tp = to + strlen(from);
    *tp-- = '\0';
    while (*from) {
	*tp-- = *from++;
    }
}

FUNCTION(fun_reverse)
{
    /* Nasty bounds checking */

    if (strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff)) {
	*(fargs[0] + (LBUF_SIZE - (*bufc - buff))) = '\0';
    }
    do_reverse(fargs[0], *bufc);
    *bufc += strlen(fargs[0]);
}

FUNCTION(fun_revwords)
{
	char *temp, *tp, *t1, sep;
	int first;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	varargs_preamble("REVWORDS", 2);
	temp = alloc_lbuf("fun_revwords");

	/* Nasty bounds checking */

	if (strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff)) {
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff))) = '\0';
	}

	/* Reverse the whole string */

	do_reverse(fargs[0], temp);

	/* Now individually reverse each word in the string.  This will
	 * undo the reversing of the words (so the words themselves are
	 * forwards again. 
	 */

	tp = temp;
	first = 1;
	while (tp) {
		if (!first)
			safe_chr(sep, buff, bufc);
		t1 = split_token(&tp, sep);
		do_reverse(t1, *bufc);
		*bufc += strlen(t1);
		first = 0;
	}
	free_lbuf(temp);
}

/* ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified string.
 */

FUNCTION(fun_after)
{
	char *bp, *cp, *mp;
	int mlen;

	if (nfargs == 0) {
		return;
	}
	if (!fn_range_check("AFTER", nfargs, 1, 2, buff, bufc))
		return;
	bp = fargs[0];
	mp = fargs[1];

	/* Sanity-check arg1 and arg2 */

	if (bp == NULL)
		bp = "";
	if (mp == NULL)
		mp = " ";
	if (!mp || !*mp)
		mp = (char *)" ";
	mlen = strlen(mp);
	if ((mlen == 1) && (*mp == ' '))
		bp = trim_space_sep(bp, ' ');

	/* Look for the target string */

	while (*bp) {

		/* Search for the first character in the target string */
	
		cp = (char *)index(bp, *mp);
		if (cp == NULL) {

			/* Not found, return empty string */

			return;
		}
		/* See if what follows is what we are looking for */

		if (!strncmp(cp, mp, mlen)) {

			/* Yup, return what follows */

			bp = cp + mlen;
			safe_str(bp, buff, bufc);
			return;
		}
		/* Continue search after found first character */

		bp = cp + 1;
	}

	/* Ran off the end without finding it */

	return;
}

FUNCTION(fun_before)
{
	char *bp, *cp, *mp, *ip;
	int mlen;

	if (nfargs == 0) {
		return;
	}
	if (!fn_range_check("BEFORE", nfargs, 1, 2, buff, bufc))
		return;

	bp = fargs[0];
	mp = fargs[1];

	/* Sanity-check arg1 and arg2 */

	if (bp == NULL)
		bp = "";
	if (mp == NULL)
		mp = " ";
	if (!mp || !*mp)
		mp = (char *)" ";
	mlen = strlen(mp);
	if ((mlen == 1) && (*mp == ' '))
		bp = trim_space_sep(bp, ' ');
	ip = bp;

	/* Look for the target string */

	while (*bp) {

		/* Search for the first character in the target string */

		cp = (char *)index(bp, *mp);
		if (cp == NULL) {

			/* Not found, return entire string */

			safe_str(ip, buff, bufc);
			return;
		}
		/* See if what follows is what we are looking for */

		if (!strncmp(cp, mp, mlen)) {

			/*
			 * Yup, return what follows 
			 */

			*cp = '\0';
			safe_str(ip, buff, bufc);
			return;
		}
		/* Continue search after found first character */

		bp = cp + 1;
	}

	/* Ran off the end without finding it */

	safe_str(ip, buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_max, fun_min: Return maximum (minimum) value.
 */

FUNCTION(fun_max)
{
    int i;
    NVAL max, val;

    if (nfargs < 1) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	max = aton(fargs[0]);
	for (i = 0; i < nfargs; i++) {
	    val = aton(fargs[i]);
	    max = (max < val) ? val : max;
	}
	fval(buff, bufc, max);
    }
    return;
}

FUNCTION(fun_min)
{
    int i;
    NVAL min, val;

    if (nfargs < 1) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	min = aton(fargs[0]);
	for (i = 0; i < nfargs; i++) {
	    val = aton(fargs[i]);
	    min = (min > val) ? val : min;
	}
	fval(buff, bufc, min);
    }
    return;
}

/* ---------------------------------------------------------------------------
 * fun_search: Search the db for things, returning a list of what matches
 */

FUNCTION(fun_search)
{
	dbref thing;
	char *bp, *nbuf;
	SEARCH searchparm;

	/* Set up for the search.  If any errors, abort. */

	if (!search_setup(player, fargs[0], &searchparm)) {
		safe_str("#-1 ERROR DURING SEARCH", buff, bufc);
		return;
	}
	/* Do the search and report the results */

	olist_push();
	search_perform(player, cause, &searchparm);
	bp = *bufc;
	nbuf = alloc_sbuf("fun_search");
	for (thing = olist_first(); thing != NOTHING; thing = olist_next()) {
		if (bp == *bufc)
			sprintf(nbuf, "#%d", thing);
		else
			sprintf(nbuf, " #%d", thing);
		safe_str(nbuf, buff, bufc);
	}
	free_sbuf(nbuf);
	olist_pop();
}

/* ---------------------------------------------------------------------------
 * fun_stats: Get database size statistics.
 */

FUNCTION(fun_stats)
{
	dbref who;
	STATS statinfo;

	if ((!fargs[0]) || !*fargs[0] || !string_compare(fargs[0], "all")) {
		who = NOTHING;
	} else {
		who = lookup_player(player, fargs[0], 1);
		if (who == NOTHING) {
			safe_str("#-1 NOT FOUND", buff, bufc);
			return;
		}
	}
	if (!get_stats(player, who, &statinfo)) {
		safe_str("#-1 ERROR GETTING STATS", buff, bufc);
		return;
	}
	safe_tprintf_str(buff, bufc, "%d %d %d %d %d %d",
			 statinfo.s_total, statinfo.s_rooms,
			 statinfo.s_exits, statinfo.s_things,
			 statinfo.s_players, statinfo.s_garbage);
}

/* ---------------------------------------------------------------------------
 * fun_merge:  given two strings and a character, merge the two strings
 *   by replacing characters in string1 that are the same as the given 
 *   character by the corresponding character in string2 (by position).
 *   The strings must be of the same length.
 */

FUNCTION(fun_merge)
{
	char *str, *rep;
	char c;

	/* do length checks first */

	if (strlen(fargs[0]) != strlen(fargs[1])) {
		safe_str("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufc);
		return;
	}
	if (strlen(fargs[2]) > 1) {
		safe_str("#-1 TOO MANY CHARACTERS", buff, bufc);
		return;
	}
	/* find the character to look for. null character is considered a
	 * space 
	 */

	if (!*fargs[2])
		c = ' ';
	else
		c = *fargs[2];

	/* walk strings, copy from the appropriate string */

	for (str = fargs[0], rep = fargs[1];
	     *str && *rep && ((*bufc - buff) < (LBUF_SIZE - 1));
	     str++, rep++, (*bufc)++) {
		if (*str == c)
			**bufc = *rep;
		else
			**bufc = *str;
	}

	return;
}

/* ---------------------------------------------------------------------------
 * fun_splice: similar to MERGE(), eplaces by word instead of by character.
 */

FUNCTION(fun_splice)
{
	char *p1, *p2, *q1, *q2, *bb_p, sep, osep;
	int words, i;

	svarargs_preamble("SPLICE", 5);

	/* length checks */

	if (countwords(fargs[2], sep) > 1) {
		safe_str("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}
	words = countwords(fargs[0], sep);
	if (words != countwords(fargs[1], sep)) {
		safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}
	/* loop through the two lists */

	p1 = fargs[0];
	q1 = fargs[1];
	bb_p = *bufc;
	for (i = 0; i < words; i++) {
		p2 = split_token(&p1, sep);
		q2 = split_token(&q1, sep);
		if (*bufc != bb_p) {
		    print_sep(osep, buff, bufc);
		}
		if (!strcmp(p2, fargs[2]))
			safe_str(q2, buff, bufc);	/* replace */
		else
			safe_str(p2, buff, bufc);	/* copy */
	}
}

/* ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

FUNCTION(fun_repeat)
{
    int times, len, i;
    char *max;

    times = atoi(fargs[1]);
    if ((times < 1) || (fargs[0] == NULL) || (!*fargs[0])) {
	return;
    } else if (times == 1) {
	safe_str(fargs[0], buff, bufc);
    } else {

	/* We must check all of these things rather than just
	 * multiplying length by times, because of the possibility
	 * of an integer overflow.
	 */

	len = strlen(fargs[0]);

	if ((len > LBUF_SIZE - 1) ||
	    (times > LBUF_SIZE - 1) ||
	    (len * times > LBUF_SIZE - 1)) {

	    safe_str("#-1 STRING TOO LONG", buff, bufc);
	    
	} else {
	    max = buff + LBUF_SIZE;
	    for (i = 0; i < times && (*bufc < max); i++)
		safe_known_str(fargs[0], len, buff, bufc);
	}
    }
}

/* ---------------------------------------------------------------------------
 * fun_loop and fun_parse exist for reasons of backwards compatibility.
 * See notes on fun_iter for the explanation.
 */

static void perform_loop(buff, bufc, player, cause, list, exprstr,
			 cargs, ncargs, sep, osep, flag)
    char *buff, **bufc;
    dbref player, cause;
    char *list, *exprstr;
    char *cargs[];
    int ncargs;
    char sep, osep;
    int flag;			/* 0 is parse(), 1 is loop() */
{
    char *curr, *objstring, *buff2, *buff3, *cp, *dp, *str, *result, *bb_p;
    char tbuf[8];
    int number = 0;

    dp = cp = curr = alloc_lbuf("perform_loop.1");
    str = list;
    exec(curr, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	 cargs, ncargs);
    *dp = '\0';
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
	free_lbuf(curr);
	return;
    }

    bb_p = *bufc;

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
	if (!flag && (*bufc != bb_p)) {
	    print_sep(osep, buff, bufc);
	}
	number++;
	objstring = split_token(&cp, sep);
	buff2 = replace_string(BOUND_VAR, objstring, exprstr);
	ltos(tbuf, number);
	buff3 = replace_string(LISTPLACE_VAR, tbuf, buff2);
	str = buff3;
	if (!flag) {
	    exec(buff, bufc, 0, player, cause,
		 EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	} else {
	    dp = result = alloc_lbuf("perform_loop.2");
	    exec(result, &dp, 0, player, cause,
		 EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	    *dp = '\0';
	    notify(cause, result);
	    free_lbuf(result);
	}
	free_lbuf(buff2);
	free_lbuf(buff3);
    }
    free_lbuf(curr);
}

FUNCTION(fun_parse)
{
    char sep, osep;

    evarargs_preamble("PARSE", 2, 4);
    perform_loop(buff, bufc, player, cause, fargs[0], fargs[1],
		 cargs, ncargs, sep, osep, 0);
}

FUNCTION(fun_loop)
{
    char sep;

    varargs_preamble("LOOP", 3);
    perform_loop(buff, bufc, player, cause, fargs[0], fargs[1],
		 cargs, ncargs, sep, ' ', 1);
}

/* ---------------------------------------------------------------------------
 * fun_iter() and fun_list() parse an expression, substitute elements of
 * a list, one at a time, using the '##' replacement token. Uses of these
 * functions can be nested.
 * In older versions of MUSH, these functions could not be nested.
 * parse() and loop() exist for reasons of backwards compatibility,
 * since the peculiarities of the way substitutions were done in the string
 * replacements make it necessary to provide some way of doing backwards
 * compatibility, in order to avoid breaking a lot of code that relies upon
 * particular patterns of necessary escaping.
 */

static void perform_iter(buff, bufc, player, cause, list, exprstr,
			 cargs, ncargs, sep, osep, flag)
    char *buff, **bufc;
    dbref player, cause;
    char *list, *exprstr;
    char *cargs[];
    int ncargs;
    char sep, osep;
    int flag;			/* 0 is iter(), 1 is list() */
{
    char *list_str, *lp, *str, *input_p, *bb_p, *save_token, *work_buf;
    char *dp, *result;
    int save_num;

    /* The list argument is unevaluated. Go evaluate it. */

    input_p = lp = list_str = alloc_lbuf("perform_iter.list");
    str = list;
    exec(list_str, &lp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	 cargs, ncargs);
    *lp = '\0';
    input_p = trim_space_sep(input_p, sep);
    if (!*input_p) {
	free_lbuf(list_str);
	return;
    }

    mudstate.in_loop++;
    save_token = mudstate.loop_token;
    save_num = mudstate.loop_number;
    mudstate.loop_number = 0;

    bb_p = *bufc;

    while (input_p && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
	if (!flag && (*bufc != bb_p)) {
	    print_sep(osep, buff, bufc);
	}
	mudstate.loop_token = split_token(&input_p, sep);
	mudstate.loop_number++;
	work_buf = alloc_lbuf("perform_iter.eval");
	strcpy(work_buf, exprstr); /* we might nibble this */
	str = work_buf;
	if (!flag) {
	    exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		 &str, cargs, ncargs);
	} else {
	    dp = result = alloc_lbuf("perform_iter.out");
	    exec(result, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		 &str, cargs, ncargs);
	    *dp = '\0';
	    notify(cause, result);
	    free_lbuf(result);
	}
	free_lbuf(work_buf);
    }

    free_lbuf(list_str);
    mudstate.loop_token = save_token;
    mudstate.loop_number = save_num;
    mudstate.in_loop--;
}

FUNCTION(fun_iter)
{
    char sep, osep;

    evarargs_preamble("ITER", 2, 4);
    perform_iter(buff, bufc, player, cause, fargs[0], fargs[1],
		 cargs, ncargs, sep, osep, 0);
}

FUNCTION(fun_list)
{
    char sep;

    varargs_preamble("LIST", 3);
    perform_iter(buff, bufc, player, cause, fargs[0], fargs[1],
		 cargs, ncargs, sep, ' ', 1);
}

/* ---------------------------------------------------------------------------
 * fun_fold: iteratively eval an attrib with a list of arguments
 *        and an optional base case.  With no base case, the first list element
 *    is passed as %0 and the second is %1.  The attrib is then evaluated
 *    with these args, the result is then used as %0 and the next arg is
 *    %1 and so it goes as there are elements left in the list.  The
 *    optinal base case gives the user a nice starting point.
 *
 *    > &REP_NUM object=[%0][repeat(%1,%1)]
 *    > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 *    You say "->122333444455555"
 *
 *      NOTE: To use added list separator, you must use base case!
 */

FUNCTION(fun_fold)
{
	dbref aowner, thing;
	int aflags, alen, anum;
	ATTR *ap;
	char *atext, *result, *curr, *bp, *str, *cp, *atextbuf, *clist[2],
	*rstore, sep;

	/* We need two to four arguements only */

	mvarargs_preamble("FOLD", 2, 4);

	/* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

	if (parse_attrib(player, fargs[0], &thing, &anum)) {
		if ((anum == NOTHING) || (!Good_obj(thing)))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(fargs[0]);
	}

	/* Make sure we got a good attribute */

	if (!ap) {
		return;
	}
	/* Use it if we can access it, otherwise return an error. */

	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!atext) {
		return;
	} else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
		free_lbuf(atext);
		return;
	}
	/* Evaluate it using the rest of the passed function args */

	cp = curr = fargs[1];
	atextbuf = alloc_lbuf("fun_fold");
	strcpy(atextbuf, atext);

	/* may as well handle first case now */

	if ((nfargs >= 3) && (fargs[2])) {
		clist[0] = fargs[2];
		clist[1] = split_token(&cp, sep);
		result = bp = alloc_lbuf("fun_fold");
		str = atextbuf;
		exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		     &str, clist, 2);
		*bp = '\0';
	} else {
		clist[0] = split_token(&cp, sep);
		clist[1] = split_token(&cp, sep);
		result = bp = alloc_lbuf("fun_fold");
		str = atextbuf;
		exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		     &str, clist, 2);
		*bp = '\0';
	}

	rstore = result;
	result = NULL;

	while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
		clist[0] = rstore;
		clist[1] = split_token(&cp, sep);
		strcpy(atextbuf, atext);
		result = bp = alloc_lbuf("fun_fold");
		str = atextbuf;
		exec(result, &bp, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 2);
		*bp = '\0';
		strcpy(rstore, result);
		free_lbuf(result);
	}
	safe_str(rstore, buff, bufc);
	free_lbuf(rstore);
	free_lbuf(atext);
	free_lbuf(atextbuf);
}

/* ---------------------------------------------------------------------------
 * fun_filter: iteratively perform a function with a list of arguments
 *              and return the arg, if the function evaluates to TRUE using the 
 *      arg.
 *
 *      > &IS_ODD object=mod(%0,2)
 *      > say filter(object/is_odd,1 2 3 4 5)
 *      You say "1 3 5"
 *      > say filter(object/is_odd,1-2-3-4-5,-)
 *      You say "1-3-5"
 *
 *  NOTE:  If you specify a separator it is used to delimit returned list
 */

static void handle_filter(player, cause, arg_func, arg_list, buff, bufc,
			  sep, osep, flag)
    dbref player, cause;
    char *arg_func, *arg_list;
    char *buff;
    char **bufc;
    char sep, osep;
    int flag;			/* 0 is filter(), 1 is filterbool() */
{
	dbref aowner, thing;
	int aflags, alen, anum;
	ATTR *ap;
	char *atext, *result, *curr, *objstring, *bp, *str, *cp, *atextbuf;
	char *bb_p;

	/* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

	if (parse_attrib(player, arg_func, &thing, &anum)) {
		if ((anum == NOTHING) || (!Good_obj(thing)))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(arg_func);
	}

	/* Make sure we got a good attribute */

	if (!ap) {
		return;
	}
	/*
	 * Use it if we can access it, otherwise return an error. 
	 */

	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!atext) {
		return;
	} else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
		free_lbuf(atext);
		return;
	}
	/* Now iteratively eval the attrib with the argument list */

	cp = curr = trim_space_sep(arg_list, sep);
	atextbuf = alloc_lbuf("fun_filter");
	bb_p = *bufc;
	while (cp) {
		objstring = split_token(&cp, sep);
		strcpy(atextbuf, atext);
		result = bp = alloc_lbuf("fun_filter");
		str = atextbuf;
		exec(result, &bp, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL, &str, &objstring, 1);
		*bp = '\0';
		if ((!flag && (*result == '1')) || (flag && xlate(result))) {
		        if (*bufc != bb_p) {
			    print_sep(osep, buff, bufc);
			}
			safe_str(objstring, buff, bufc);
		}
		free_lbuf(result);
	}
	free_lbuf(atext);
	free_lbuf(atextbuf);
}

FUNCTION(fun_filter)
{
	char sep, osep;

	svarargs_preamble("FILTER", 4);
	handle_filter(player, cause, fargs[0], fargs[1], buff, bufc,
		      sep, osep, 0);
}

FUNCTION(fun_filterbool)
{
	char sep, osep;

	svarargs_preamble("FILTERBOOL", 4);
	handle_filter(player, cause, fargs[0], fargs[1], buff, bufc,
		      sep, osep, 1);
}

/* ---------------------------------------------------------------------------
 * fun_map: iteratively evaluate an attribute with a list of arguments.
 *  > &DIV_TWO object=fdiv(%0,2)
 *  > say map(1 2 3 4 5,object/div_two)
 *  You say "0.5 1 1.5 2 2.5"
 *  > say map(object/div_two,1-2-3-4-5,-)
 *  You say "0.5-1-1.5-2-2.5"
 *
 */

FUNCTION(fun_map)
{
	dbref aowner, thing;
	int aflags, alen, anum;
	ATTR *ap;
	char *atext, *objstring, *str, *cp, *atextbuf, *bb_p, sep, osep;

	svarargs_preamble("MAP", 4);

	/* If we don't have anything for a second arg, don't bother. */
	if (!fargs[1] || !*fargs[1])
	        return;

	/* Two possibilities for the second arg: <obj>/<attr> and <attr>. */

	if (parse_attrib(player, fargs[0], &thing, &anum)) {
		if ((anum == NOTHING) || (!Good_obj(thing)))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(fargs[0]);
	}

	/* Make sure we got a good attribute */

	if (!ap) {
		return;
	}
	/* Use it if we can access it, otherwise return an error. */

	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!atext) {
		return;
	} else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
		free_lbuf(atext);
		return;
	}
	/* now process the list one element at a time */

	cp = trim_space_sep(fargs[1], sep);
	atextbuf = alloc_lbuf("fun_map");
	bb_p = *bufc;
	while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
	        if (*bufc != bb_p) {
		    print_sep(osep, buff, bufc);
		}
		objstring = split_token(&cp, sep);
		strcpy(atextbuf, atext);
		str = atextbuf;
		exec(buff, bufc, 0, player, cause,
		     EV_STRIP | EV_FCHECK | EV_EVAL, &str, &objstring, 1);
	}
	free_lbuf(atext);
	free_lbuf(atextbuf);
}

/* ---------------------------------------------------------------------------
 * fun_while: Evaluate a list until a termination condition is met:
 * while(EVAL_FN,CONDITION_FN,foo|flibble|baz|meep,1,|,-)
 * where EVAL_FN is "[strlen(%0)]" and CONDITION_FN is "[strmatch(%0,baz)]"
 * would result in '3-7-3' being returned.
 * The termination condition is an EXACT not wild match.
 */

FUNCTION(fun_while)
{
    char sep, osep;
    dbref aowner1, thing1, aowner2, thing2;
    int aflags1, aflags2, anum1, anum2, alen1, alen2;
    int is_same;
    ATTR *ap;
    char *atext1, *atext2, *atextbuf, *condbuf;
    char *objstring, *cp, *str, *dp, *savep, *bb_p;

    svarargs_preamble("WHILE", 6);

    /* If our third arg is null (empty list), don't bother. */

    if (!fargs[2] || !*fargs[2])
	return;

    /* Our first and second args can be <obj>/<attr> or just <attr>.
     * Use them if we can access them, otherwise return an empty string.
     *
     * Note that for user-defined attributes, atr_str() returns a pointer
     * to a static, and that therefore we have to be careful about what
     * we're doing.
     */

    if (parse_attrib(player, fargs[0], &thing1, &anum1)) {
	if ((anum1 == NOTHING) || !Good_obj(thing1))
	    ap = NULL;
	else
	    ap = atr_num(anum1);
    } else {
	thing1 = player;
	ap = atr_str(fargs[0]);
    }
    if (!ap)
	return;
    atext1 = atr_pget(thing1, ap->number, &aowner1, &aflags1, &alen1);
    if (!atext1) {
	return;
    } else if (!*atext1 || !See_attr(player, thing1, ap, aowner1, aflags1)) {
	free_lbuf(atext1);
	return;
    }

    if (parse_attrib(player, fargs[1], &thing2, &anum2)) {
	if ((anum2 == NOTHING) || !Good_obj(thing2))
	    ap = NULL;
	else
	    ap = atr_num(anum2);
    } else {
	thing2 = player;
	ap = atr_str(fargs[1]);
    }
    if (!ap) {
	free_lbuf(atext1);	/* we allocated this, remember? */
	return;
    }
    atext2 = atr_pget(thing2, ap->number, &aowner2, &aflags2, &alen2);
    if (!atext2) {
	free_lbuf(atext1);
	return;
    } else if (!*atext2 || !See_attr(player, thing2, ap, aowner2, aflags2)) {
	free_lbuf(atext1);
	free_lbuf(atext2);
	return;
    }

    /* If our evaluation and condition are the same, we can save ourselves
     * some time later.
     */
    if (!strcmp(atext1, atext2))
	is_same = 1;
    else 
	is_same = 0;

    /* Process the list one element at a time. */

    cp = trim_space_sep(fargs[2], sep);
    atextbuf = alloc_lbuf("fun_while.eval");
    if (!is_same)
	condbuf = alloc_lbuf("fun_while.cond");
    bb_p = *bufc;
    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
	if (*bufc != bb_p) {
	    print_sep(osep, buff, bufc);
	}
	objstring = split_token(&cp, sep);
	strcpy(atextbuf, atext1);
	str = atextbuf;
	savep = *bufc;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, &objstring, 1);
	if (is_same) {
	    if (!strcmp(savep, fargs[3]))
		break;
	} else {
	    strcpy(condbuf, atext2);
	    dp = str = savep = condbuf;
	    exec(condbuf, &dp, 0, player, cause,
		 EV_STRIP | EV_FCHECK | EV_EVAL, &str, &objstring, 1);
	    if (!strcmp(savep, fargs[3]))
		break;
	}
    }
    free_lbuf(atext1);
    free_lbuf(atext2);
    free_lbuf(atextbuf);
    if (!is_same)
	free_lbuf(condbuf);
}


/* ---------------------------------------------------------------------------
 * fun_edit: Edit text.
 */

FUNCTION(fun_edit)
{
    char *tstr;

    edit_string(fargs[0], &tstr, fargs[1], fargs[2]);
    safe_str(tstr, buff, bufc);
    free_lbuf(tstr);
}

/* ---------------------------------------------------------------------------
 * fun_locate: Search for things with the perspective of another obj.
 */

FUNCTION(fun_locate)
{
	int pref_type, check_locks, verbose, multiple;
	dbref thing, what;
	char *cp;

	pref_type = NOTYPE;
	check_locks = verbose = multiple = 0;

	/* Find the thing to do the looking, make sure we control it. */

	if (See_All(player))
		thing = match_thing(player, fargs[0]);
	else
		thing = match_controlled(player, fargs[0]);
	if (!Good_obj(thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	/* Get pre- and post-conditions and modifiers */

	for (cp = fargs[2]; *cp; cp++) {
		switch (*cp) {
		case 'E':
			pref_type = TYPE_EXIT;
			break;
		case 'L':
			check_locks = 1;
			break;
		case 'P':
			pref_type = TYPE_PLAYER;
			break;
		case 'R':
			pref_type = TYPE_ROOM;
			break;
		case 'T':
			pref_type = TYPE_THING;
			break;
		case 'V':
			verbose = 1;
			break;
		case 'X':
			multiple = 1;
			break;
		}
	}

	/* Set up for the search */

	if (check_locks)
		init_match_check_keys(thing, fargs[1], pref_type);
	else
		init_match(thing, fargs[1], pref_type);

	/* Search for each requested thing */

	for (cp = fargs[2]; *cp; cp++) {
		switch (*cp) {
		case 'a':
			match_absolute();
			break;
		case 'c':
			match_carried_exit_with_parents();
			break;
		case 'e':
			match_exit_with_parents();
			break;
		case 'h':
			match_here();
			break;
		case 'i':
			match_possession();
			break;
		case 'm':
			match_me();
			break;
		case 'n':
			match_neighbor();
			break;
		case 'p':
			match_player();
			break;
		case '*':
			match_everything(MAT_EXIT_PARENTS);
			break;
		}
	}

	/* Get the result and return it to the caller */

	if (multiple)
		what = last_match_result();
	else
		what = match_result();

	if (verbose)
		(void)match_status(player, what);

	safe_dbref(buff, bufc, what);
}

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

/* ---------------------------------------------------------------------------
 * fun_space: Make spaces.
 */

FUNCTION(fun_space)
{
	int num, max;
	char *cp;

	if (!fargs[0] || !(*fargs[0])) {
		num = 1;
	} else {
		num = atoi(fargs[0]);
	}

	if (num < 1) {
		/* If negative or zero spaces return a single space,
		 * -except- allow 'space(0)' to return "" for calculated
		 * padding 
		 */

		if (!is_integer(fargs[0]) || (num != 0)) {
			num = 1;
		}
	}

	max = LBUF_SIZE - 1 - (*bufc - buff);
	num = (num > max) ? max : num;
	bcopy(space_buffer, *bufc, num);
	*bufc += num;
	**bufc = '\0';
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
 * fun_sort: Sort lists.
 */

typedef struct f_record f_rec;
typedef struct i_record i_rec;

struct f_record {
	double data;
	char *str;
};

struct i_record {
	long data;
	char *str;
};

static int a_comp(s1, s2)
const void *s1, *s2;
{
	return strcmp(*(char **)s1, *(char **)s2);
}

static int f_comp(s1, s2)
const void *s1, *s2;
{
	if (((f_rec *) s1)->data > ((f_rec *) s2)->data)
		return 1;
	if (((f_rec *) s1)->data < ((f_rec *) s2)->data)
		return -1;
	return 0;
}

static int i_comp(s1, s2)
const void *s1, *s2;
{
	if (((i_rec *) s1)->data > ((i_rec *) s2)->data)
		return 1;
	if (((i_rec *) s1)->data < ((i_rec *) s2)->data)
		return -1;
	return 0;
}

static void do_asort(s, n, sort_type)
char *s[];
int n, sort_type;
{
	int i;
	f_rec *fp;
	i_rec *ip;

	switch (sort_type) {
	case ALPHANUM_LIST:
		qsort((void *)s, n, sizeof(char *), a_comp);

		break;
	case NUMERIC_LIST:
		ip = (i_rec *) XMALLOC(n * sizeof(i_rec), "do_asort");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = atoi(s[i]);
		}
		qsort((void *)ip, n, sizeof(i_rec), i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		free(ip);
		break;
	case DBREF_LIST:
		ip = (i_rec *) XMALLOC(n * sizeof(i_rec), "do_asort.2");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
		}
		qsort((void *)ip, n, sizeof(i_rec), i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		free(ip);
		break;
	case FLOAT_LIST:
		fp = (f_rec *) XMALLOC(n * sizeof(f_rec), "do_asort.3");
		for (i = 0; i < n; i++) {
			fp[i].str = s[i];
			fp[i].data = atof(s[i]);
		}
		qsort((void *)fp, n, sizeof(f_rec), f_comp);
		for (i = 0; i < n; i++) {
			s[i] = fp[i].str;
		}
		free(fp);
		break;
	}
}

FUNCTION(fun_sort)
{
	int nitems, sort_type;
	char *list, sep, osep;
	char *ptrs[LBUF_SIZE / 2];

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}

	if (!fn_range_check("SORT", nfargs, 1, 4, buff, bufc))
		return;
	if (!delim_check(fargs, nfargs, 3, &sep, buff, bufc, 0,
	     player, cause, cargs, ncargs, 0))
		return;
	if (nfargs < 4)
		osep = sep;
	else if (!delim_check(fargs, nfargs, 4, &osep, buff, bufc, 0,
	         player, cause, cargs, ncargs, 1))
		return;


	/* Convert the list to an array */

	list = alloc_lbuf("fun_sort");
	strcpy(list, fargs[0]);
	nitems = list2arr(ptrs, LBUF_SIZE / 2, list, sep);
	sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
	do_asort(ptrs, nitems, sort_type);
	arr2list(ptrs, nitems, buff, bufc, osep);
	free_lbuf(list);
}

/* ---------------------------------------------------------------------------
 * fun_setunion, fun_setdiff, fun_setinter: Set management.
 */

#define	SET_UNION	1
#define	SET_INTERSECT	2
#define	SET_DIFF	3

static void handle_sets(fargs, buff, bufc, oper, sep, osep)
char *fargs[], *buff, **bufc, sep, osep;
int oper;
{
	char *list1, *list2, *oldp, *bb_p;
	char *ptrs1[LBUF_SIZE], *ptrs2[LBUF_SIZE];
	int i1, i2, n1, n2, val;

	list1 = alloc_lbuf("fun_setunion.1");
	strcpy(list1, fargs[0]);
	n1 = list2arr(ptrs1, LBUF_SIZE, list1, sep);
	do_asort(ptrs1, n1, ALPHANUM_LIST);

	list2 = alloc_lbuf("fun_setunion.2");
	strcpy(list2, fargs[1]);
	n2 = list2arr(ptrs2, LBUF_SIZE, list2, sep);
	do_asort(ptrs2, n2, ALPHANUM_LIST);

	i1 = i2 = 0;
	bb_p = oldp = *bufc;
	**bufc = '\0';

	switch (oper) {
	case SET_UNION:	/* Copy elements common to both lists */

		/* Handle case of two identical single-element lists */

		if ((n1 == 1) && (n2 == 1) &&
		    (!strcmp(ptrs1[0], ptrs2[0]))) {
			safe_str(ptrs1[0], buff, bufc);
			break;
		}
		/* Process until one list is empty */

		while ((i1 < n1) && (i2 < n2)) {

			/* Skip over duplicates */

			if ((i1 > 0) || (i2 > 0)) {
				while ((i1 < n1) && !strcmp(ptrs1[i1],
							    oldp))
					i1++;
				while ((i2 < n2) && !strcmp(ptrs2[i2],
							    oldp))
					i2++;
			}
			/* Compare and copy */

			if ((i1 < n1) && (i2 < n2)) {
			        if (*bufc != bb_p) {
				        print_sep(osep, buff, bufc);
				}
				oldp = *bufc;
				if (strcmp(ptrs1[i1], ptrs2[i2]) < 0) {
					safe_str(ptrs1[i1], buff, bufc);
					i1++;
				} else {
					safe_str(ptrs2[i2], buff, bufc);
					i2++;
				}
				**bufc = '\0';
			}
		}

		/* Copy rest of remaining list, stripping duplicates */

		for (; i1 < n1; i1++) {
			if (strcmp(oldp, ptrs1[i1])) {
			        if (*bufc != bb_p) {
					print_sep(osep, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}
		for (; i2 < n2; i2++) {
			if (strcmp(oldp, ptrs2[i2])) {
			        if (*bufc != bb_p) {
				    print_sep(osep, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs2[i2], buff, bufc);
				**bufc = '\0';
			}
		}
		break;
	case SET_INTERSECT:	/* Copy elements not in both lists */

		while ((i1 < n1) && (i2 < n2)) {
			val = strcmp(ptrs1[i1], ptrs2[i2]);
			if (!val) {

				/* Got a match, copy it */

			        if (*bufc != bb_p) {
					print_sep(osep, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs1[i1], buff, bufc);
				i1++;
				i2++;
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
					i1++;
				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
					i2++;
			} else if (val < 0) {
				i1++;
			} else {
				i2++;
			}
		}
		break;
	case SET_DIFF:		/* Copy elements unique to list1 */

		while ((i1 < n1) && (i2 < n2)) {
			val = strcmp(ptrs1[i1], ptrs2[i2]);
			if (!val) {

				/* Got a match, increment pointers */

				oldp = ptrs1[i1];
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
					i1++;
				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
					i2++;
			} else if (val < 0) {

				/* Item in list1 not in list2, copy */

			        if (*bufc != bb_p) {
					print_sep(osep, buff, bufc);
				}
				safe_str(ptrs1[i1], buff, bufc);
				oldp = ptrs1[i1];
				i1++;
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
					i1++;
			} else {

				/* Item in list2 but not in list1, discard */

				oldp = ptrs2[i2];
				i2++;
				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
					i2++;
			}
		}

		/* Copy remainder of list1 */

		while (i1 < n1) {
		        if (*bufc != bb_p) {
			    print_sep(osep, buff, bufc);
			}
			safe_str(ptrs1[i1], buff, bufc);
			oldp = ptrs1[i1];
			i1++;
			while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
				i1++;
		}
	}
	free_lbuf(list1);
	free_lbuf(list2);
	return;
}

FUNCTION(fun_setunion)
{
	char sep, osep;

	svarargs_preamble("SETUNION", 4);
	handle_sets(fargs, buff, bufc, SET_UNION, sep, osep);
	return;
}

FUNCTION(fun_setdiff)
{
	char sep, osep;

	svarargs_preamble("SETDIFF", 4);
	handle_sets(fargs, buff, bufc, SET_DIFF, sep, osep);
	return;
}

FUNCTION(fun_setinter)
{
	char sep, osep;

	svarargs_preamble("SETINTER", 4);
	handle_sets(fargs, buff, bufc, SET_INTERSECT, sep, osep);
	return;
}

/* ---------------------------------------------------------------------------
 * rjust, ljust, center: Justify or center text, specifying fill character
 */

FUNCTION(fun_ljust)
{
	int spaces, i, max;
	char *tp;
	char sep;

	varargs_preamble("LJUST", 3);
	spaces = atoi(fargs[1]) - strlen((char *)strip_ansi(fargs[0]));

	/* Sanitize number of spaces */

	if (spaces <= 0) {
		/* no padding needed, just return string */
		safe_str(fargs[0], buff, bufc);
		return;
	}

	safe_str(fargs[0], buff, bufc);

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;
	if (sep == ' ') {
	    bcopy(space_buffer, tp, spaces);
	    tp += spaces;
	} else {
	    for (i = 0; i < spaces; i++)
		*tp++ = sep;
	}
	*tp = '\0';
	*bufc = tp;
}

FUNCTION(fun_rjust)
{
	int spaces, i, max;
	char *tp;
	char sep;

	varargs_preamble("RJUST", 3);
	spaces = atoi(fargs[1]) - strlen((char *)strip_ansi(fargs[0]));

	/* Sanitize number of spaces */

	if (spaces <= 0) {
		/* no padding needed, just return string */
		safe_str(fargs[0], buff, bufc);
		return;
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;
	if (sep == ' ') {
	    bcopy(space_buffer, tp, spaces);
	    tp += spaces;
	} else {
	    for (i = 0; i < spaces; i++)
		*tp++ = sep;
	}
	*bufc = tp;
	safe_str(fargs[0], buff, bufc);
}

FUNCTION(fun_center)
{
	char sep;
	char *tp;
	int i, len, lead_chrs, trail_chrs, width, max;

	varargs_preamble("CENTER", 3);

	width = atoi(fargs[1]);
	len = strlen((char *)strip_ansi(fargs[0]));

	width = (width > LBUF_SIZE - 1) ? LBUF_SIZE - 1 : width;
	if (len >= width) {
		safe_str(fargs[0], buff, bufc);
		return;
	}

	lead_chrs = (width / 2) - (len / 2) + .5;
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	lead_chrs = (lead_chrs > max) ? max : lead_chrs;
	if (sep == ' ') {
	    bcopy(space_buffer, tp, lead_chrs);
	    tp += lead_chrs;
	} else {
	    for (i = 0; i < lead_chrs; i++)
		*tp++ = sep;
	}
	*bufc = tp;

	safe_str(fargs[0], buff, bufc);

	trail_chrs = width - lead_chrs - len;
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff);
	trail_chrs = (trail_chrs > max) ? max : trail_chrs;
	if (sep == ' ') {
	    bcopy(space_buffer, tp, trail_chrs);
	    tp += trail_chrs;
	} else {
	    for (i = 0; i < lead_chrs; i++)
		*tp++ = sep;
	}
	*tp = '\0';
	*bufc = tp;
}

/* ---------------------------------------------------------------------------
 * setq, setr, r: set and read global registers.
 */

FUNCTION(fun_setq)
{
	int regnum, len;

	regnum = atoi(fargs[0]);
	if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
		safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
	}
		
	if (!mudstate.global_regs[regnum])
	    mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
	len = strlen(fargs[1]);
	bcopy(fargs[1], mudstate.global_regs[regnum], len + 1);
	mudstate.glob_reg_len[regnum] = len;
}

FUNCTION(fun_setr)
{
	int regnum, len;

	regnum = atoi(fargs[0]);
	if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
		safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
		return;
	}

	if (!mudstate.global_regs[regnum])
	    mudstate.global_regs[regnum] = alloc_lbuf("fun_setr");
	len = strlen(fargs[1]);
	bcopy(fargs[1], mudstate.global_regs[regnum], len + 1);
	mudstate.glob_reg_len[regnum] = len;
	safe_known_str(fargs[1], len, buff, bufc);
}

FUNCTION(fun_r)
{
	int regnum;

	regnum = atoi(fargs[0]);
	if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
		safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
	} else if (mudstate.global_regs[regnum]) {
		safe_known_str(mudstate.global_regs[regnum],
			       mudstate.glob_reg_len[regnum], buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * isword: is every character in the argument a letter?
 */
  
FUNCTION(fun_isword) {
char *p;
      
	for (p = fargs[0]; *p; p++) {
		if (!isalpha(*p)) {
			safe_chr('0', buff, bufc);
			return;
		}
	}
	safe_chr('1', buff, bufc);
}
                                                          

/* ---------------------------------------------------------------------------
 * isnum: is the argument a number?
 */

FUNCTION(fun_isnum)
{
	safe_chr((is_number(fargs[0]) ? '1' : '0'), buff, bufc);
}

/* ---------------------------------------------------------------------------
 * isdbref: is the argument a valid dbref?
 */

FUNCTION(fun_isdbref)
{
	char *p;
	dbref dbitem;

	p = fargs[0];
	if (*p++ == NUMBER_TOKEN) {
	    if (*p) {		/* just the string '#' won't do! */
		dbitem = parse_dbref(p);
		if (Good_obj(dbitem)) {
			safe_chr('1', buff, bufc);
			return;
		}
	    }
	}
	safe_chr('0', buff, bufc);
}

/* ---------------------------------------------------------------------------
 * trim: trim off unwanted white space.
 */

FUNCTION(fun_trim)
{
	char *p, *lastchar, *q, sep;
	int trim;

	if (nfargs == 0) {
		return;
	}
	mvarargs_preamble("TRIM", 1, 3);
	if (nfargs >= 2) {
		switch (ToLower(*fargs[1])) {
		case 'l':
			trim = 1;
			break;
		case 'r':
			trim = 2;
			break;
		default:
			trim = 3;
			break;
		}
	} else {
		trim = 3;
	}

	if (trim == 2 || trim == 3) {
		p = lastchar = fargs[0];
		while (*p != '\0') {
			if (*p != sep)
				lastchar = p;
			p++;
		}
		*(lastchar + 1) = '\0';
	}
	q = fargs[0];
	if (trim == 1 || trim == 3) {
		while (*q != '\0') {
			if (*q == sep)
				q++;
			else
				break;
		}
	}
	safe_str(q, buff, bufc);
}
/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * flist: List of existing functions in alphabetical order.
 */

FUN flist[] = {
{"ABS",		fun_abs,	1,  0,		CA_PUBLIC},
{"ACOS",	fun_acos,	1,  0,		CA_PUBLIC},
{"ADD",		fun_add,	0,  FN_VARARGS,	CA_PUBLIC},
{"AFTER",	fun_after,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMAX",	fun_alphamax,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMIN",	fun_alphamin,   0,  FN_VARARGS, CA_PUBLIC},
{"AND",		fun_and,	0,  FN_VARARGS, CA_PUBLIC},
{"ANDBOOL",	fun_andbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"ANDFLAGS",	fun_andflags,	2,  0,		CA_PUBLIC},
{"ANSI",        fun_ansi,       2,  0,          CA_PUBLIC},
{"APOSS",	fun_aposs,	1,  0,		CA_PUBLIC},
{"ART",		fun_art,	1,  0,		CA_PUBLIC},
{"ASIN",	fun_asin,	1,  0,		CA_PUBLIC},
{"ATAN",	fun_atan,	1,  0,		CA_PUBLIC},
{"BAND",	fun_band,	2,  0,		CA_PUBLIC},
{"BEEP",        fun_beep,       0,  0,          CA_WIZARD},
{"BEFORE",	fun_before,	0,  FN_VARARGS,	CA_PUBLIC},
{"BNAND",	fun_bnand,	2,  0,		CA_PUBLIC},
{"BOR",		fun_bor,	2,  0,		CA_PUBLIC},
{"CAPSTR",	fun_capstr,	-1, 0,		CA_PUBLIC},
{"CASE",	fun_case,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"CAT",		fun_cat,	0,  FN_VARARGS,	CA_PUBLIC},
{"CEIL",	fun_ceil,	1,  0,		CA_PUBLIC},
{"CENTER",	fun_center,	0,  FN_VARARGS,	CA_PUBLIC},
{"CHILDREN",    fun_children,   1,  0,          CA_PUBLIC},
{"CHOMP",	fun_chomp,	1,  0,		CA_PUBLIC},
{"CLEARVARS",	fun_clearvars,	0,  0,		CA_PUBLIC},
{"COLUMNS",	fun_columns,	0,  FN_VARARGS, CA_PUBLIC},
{"COMMAND",	fun_command,	0,  FN_VARARGS, CA_PUBLIC},
{"COMP",	fun_comp,	2,  0,		CA_PUBLIC},
{"CON",		fun_con,	1,  0,		CA_PUBLIC},
{"CONN",	fun_conn,	1,  0,		CA_PUBLIC},
{"CONSTRUCT",	fun_construct,	0,  FN_VARARGS,	CA_PUBLIC},
{"CONTROLS", 	fun_controls,	2,  0,		CA_PUBLIC},
{"CONVSECS",    fun_convsecs,   1,  0,		CA_PUBLIC},
{"CONVTIME",    fun_convtime,   1,  0,		CA_PUBLIC},
{"COS",		fun_cos,	1,  0,		CA_PUBLIC},
{"CREATE",      fun_create,     0,  FN_VARARGS, CA_PUBLIC},
#ifdef USE_COMSYS
{"CWHO",        fun_cwho,       1,  0,          CA_PUBLIC},
#endif
{"DEC",         fun_dec,        1,  0,          CA_PUBLIC},
{"DECRYPT",	fun_decrypt,	2,  0,		CA_PUBLIC},
{"DEFAULT",	fun_default,	2,  FN_NO_EVAL, CA_PUBLIC},
{"DELETE",	fun_delete,	3,  0,		CA_PUBLIC},
{"DESTRUCT",	fun_destruct,	1,  0,		CA_PUBLIC},
{"DIE",		fun_die,	2,  0,		CA_PUBLIC},
{"DIST2D",	fun_dist2d,	4,  0,		CA_PUBLIC},
{"DIST3D",	fun_dist3d,	6,  0,		CA_PUBLIC},
{"DIV",		fun_div,	2,  0,		CA_PUBLIC},
{"DOING",	fun_doing,	1,  0,		CA_PUBLIC},
{"DUP",		fun_dup,	0,  FN_VARARGS,	CA_PUBLIC},
{"E",		fun_e,		0,  0,		CA_PUBLIC},
{"EDEFAULT",	fun_edefault,	2,  FN_NO_EVAL, CA_PUBLIC},
{"EDIT",	fun_edit,	3,  0,		CA_PUBLIC},
{"ELEMENTS",	fun_elements,	0,  FN_VARARGS,	CA_PUBLIC},
{"ELOCK",	fun_elock,	2,  0,		CA_PUBLIC},
{"EMPTY",	fun_empty,	0,  FN_VARARGS, CA_PUBLIC},
{"ENCRYPT",	fun_encrypt,	2,  0,		CA_PUBLIC},
{"EQ",		fun_eq,		2,  0,		CA_PUBLIC},
{"ESCAPE",	fun_escape,	-1, 0,		CA_PUBLIC},
{"EXIT",	fun_exit,	1,  0,		CA_PUBLIC},
{"EXP",		fun_exp,	1,  0,		CA_PUBLIC},
{"EXTRACT",	fun_extract,	0,  FN_VARARGS,	CA_PUBLIC},
{"EVAL",        fun_eval,       0,  FN_VARARGS, CA_PUBLIC},
{"SUBEVAL",  	fun_subeval,	1,  0,		CA_PUBLIC},
{"FDIV",	fun_fdiv,	2,  0,		CA_PUBLIC},
{"FILTER",	fun_filter,	0,  FN_VARARGS,	CA_PUBLIC},
{"FILTERBOOL",	fun_filterbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"FINDABLE",	fun_findable,	2,  0,		CA_PUBLIC},
{"FIRST",	fun_first,	0,  FN_VARARGS,	CA_PUBLIC},
{"FLAGS",	fun_flags,	1,  0,		CA_PUBLIC},
{"FLOOR",	fun_floor,	1,  0,		CA_PUBLIC},
{"FOLD",	fun_fold,	0,  FN_VARARGS,	CA_PUBLIC},
{"FORCE",	fun_force,	2,  0,		CA_PUBLIC},
{"FOREACH",	fun_foreach,	0,  FN_VARARGS,	CA_PUBLIC},
{"FULLNAME",	fun_fullname,	1,  0,		CA_PUBLIC},
{"GET",		fun_get,	1,  0,		CA_PUBLIC},
{"GET_EVAL",	fun_get_eval,	1,  0,		CA_PUBLIC},
{"GRAB",	fun_grab,	0,  FN_VARARGS,	CA_PUBLIC},
{"GREP",	fun_grep,	3,  0,		CA_PUBLIC},
{"GREPI",	fun_grepi,	3,  0,		CA_PUBLIC},
{"GT",		fun_gt,		2,  0,		CA_PUBLIC},
{"GTE",		fun_gte,	2,  0,		CA_PUBLIC},
{"HASATTR",	fun_hasattr,	2,  0,		CA_PUBLIC},
{"HASATTRP",	fun_hasattrp,	2,  0,		CA_PUBLIC},
{"HASFLAG",	fun_hasflag,	2,  0,		CA_PUBLIC},
{"HASPOWER",    fun_haspower,   2,  0,          CA_PUBLIC},
{"HASTYPE",	fun_hastype,	2,  0,		CA_PUBLIC},
{"HOME",	fun_home,	1,  0,		CA_PUBLIC},
#ifdef PUEBLO_SUPPORT
{"HTML_ESCAPE",	fun_html_escape,-1, 0,		CA_PUBLIC},
{"HTML_UNESCAPE",fun_html_unescape,-1,0,	CA_PUBLIC},
#endif /* PUEBLO_SUPPORT */
{"IDLE",	fun_idle,	1,  0,		CA_PUBLIC},
{"IFELSE",      fun_ifelse,     3,  FN_NO_EVAL,          CA_PUBLIC},
{"INC",         fun_inc,        1,  0,          CA_PUBLIC},
{"INDEX",	fun_index,	4,  0,		CA_PUBLIC},
{"INSERT",	fun_insert,	0,  FN_VARARGS,	CA_PUBLIC},
{"INZONE",      fun_inzone,     1,  0,          CA_PUBLIC},
{"ISDBREF",	fun_isdbref,	1,  0,		CA_PUBLIC},
{"ISNUM",	fun_isnum,	1,  0,		CA_PUBLIC},
{"ISWORD",	fun_isword,	1,  0,		CA_PUBLIC},
{"ITEMS",	fun_items,	0,  FN_VARARGS,	CA_PUBLIC},
{"ITER",	fun_iter,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LADD",	fun_ladd,	0,  FN_VARARGS,	CA_PUBLIC},
{"LAND",	fun_land,	0,  FN_VARARGS,	CA_PUBLIC},
{"LANDBOOL",	fun_landbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"LAST",	fun_last,	0,  FN_VARARGS,	CA_PUBLIC},
{"LASTCREATE",	fun_lastcreate,	2,  0,		CA_PUBLIC},
{"LATTR",	fun_lattr,	1,  0,		CA_PUBLIC},
{"LCON",	fun_lcon,	1,  0,		CA_PUBLIC},
{"LCSTR",	fun_lcstr,	-1, 0,		CA_PUBLIC},
{"LDELETE",	fun_ldelete,	0,  FN_VARARGS,	CA_PUBLIC},
{"LEFT",	fun_left,	2,  0,		CA_PUBLIC},
{"LET",		fun_let,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LEXITS",	fun_lexits,	1,  0,		CA_PUBLIC},
{"LIST",	fun_list,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC}, 
{"LIT",		fun_lit,	1,  FN_NO_EVAL,	CA_PUBLIC},
{"LJUST",	fun_ljust,	0,  FN_VARARGS,	CA_PUBLIC},
{"LINK",	fun_link,	2,  0,		CA_PUBLIC},
{"LINSTANCES",	fun_linstances,	0,  0,		CA_PUBLIC},
{"LMAX",	fun_lmax,	0,  FN_VARARGS,	CA_PUBLIC},
{"LMIN",	fun_lmin,	0,  FN_VARARGS,	CA_PUBLIC},
{"LN",		fun_ln,		1,  0,		CA_PUBLIC},
{"LNUM",	fun_lnum,	0,  FN_VARARGS,	CA_PUBLIC},
{"LOAD",	fun_load,	0,  FN_VARARGS,	CA_PUBLIC},
{"LOC",		fun_loc,	1,  0,		CA_PUBLIC},
{"LOCATE",	fun_locate,	3,  0,		CA_PUBLIC},
{"LOCALIZE",    fun_localize,   1,  FN_NO_EVAL, CA_PUBLIC},
{"LOCK",	fun_lock,	1,  0,		CA_PUBLIC},
{"LOG",		fun_log,	0,  FN_VARARGS,	CA_PUBLIC},
{"LPARENT",	fun_lparent,	1,  0,		CA_PUBLIC}, 
{"LOOP",	fun_loop,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LOR",		fun_lor,	0,  FN_VARARGS,	CA_PUBLIC},
{"LORBOOL",	fun_lorbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"LPOS",	fun_lpos,	2,  0,		CA_PUBLIC},
{"LSTACK",	fun_lstack,	0,  FN_VARARGS, CA_PUBLIC},
{"LSTRUCTURES",	fun_lstructures, 0, 0,		CA_PUBLIC},
{"LT",		fun_lt,		2,  0,		CA_PUBLIC},
{"LTE",		fun_lte,	2,  0,		CA_PUBLIC},
{"LVARS",	fun_lvars,	0,  0,		CA_PUBLIC},
{"LWHO",	fun_lwho,	0,  0,		CA_PUBLIC},
#ifdef USE_MAIL
{"MAIL",        fun_mail,       0,  FN_VARARGS, CA_PUBLIC},
{"MAILFROM",    fun_mailfrom,   0,  FN_VARARGS, CA_PUBLIC},
#endif
{"MAP",		fun_map,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCH",	fun_match,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCHALL",	fun_matchall,	0,  FN_VARARGS,	CA_PUBLIC},
{"MAX",		fun_max,	0,  FN_VARARGS,	CA_PUBLIC},
{"MEMBER",	fun_member,	0,  FN_VARARGS,	CA_PUBLIC},
{"MERGE",	fun_merge,	3,  0,		CA_PUBLIC},
{"MID",		fun_mid,	3,  0,		CA_PUBLIC},
{"MIN",		fun_min,	0,  FN_VARARGS,	CA_PUBLIC},
{"MIX",		fun_mix,	0,  FN_VARARGS,	CA_PUBLIC},
{"MOD",		fun_mod,	2,  0,		CA_PUBLIC},
{"MODIFY",	fun_modify,	3,  0,		CA_PUBLIC},
{"MONEY",	fun_money,	1,  0,		CA_PUBLIC},
{"MUDNAME",	fun_mudname,	0,  0,		CA_PUBLIC},
{"MUL",		fun_mul,	0,  FN_VARARGS,	CA_PUBLIC},
{"MUNGE",	fun_munge,	0,  FN_VARARGS,	CA_PUBLIC},
{"NAME",	fun_name,	1,  0,		CA_PUBLIC},
{"NCOMP",	fun_ncomp,	2,  0,		CA_PUBLIC},
{"NEARBY",	fun_nearby,	2,  0,		CA_PUBLIC},
{"NEQ",		fun_neq,	2,  0,		CA_PUBLIC},
{"NEXT",	fun_next,	1,  0,		CA_PUBLIC},
{"NONZERO",	fun_nonzero,	3,  FN_NO_EVAL, CA_PUBLIC},
{"NOT",		fun_not,	1,  0,		CA_PUBLIC},
{"NOTBOOL",	fun_notbool,	1,  0,		CA_PUBLIC},
{"NULL",	fun_null,	1,  0,		CA_PUBLIC},
{"NUM",		fun_num,	1,  0,		CA_PUBLIC},
{"OBJ",		fun_obj,	1,  0,		CA_PUBLIC},
{"OBJEVAL",     fun_objeval,    2,  FN_NO_EVAL, CA_PUBLIC},
{"OBJMEM",	fun_objmem,	1,  0,		CA_PUBLIC},
{"OR",		fun_or,		0,  FN_VARARGS, CA_PUBLIC},
{"ORBOOL",	fun_orbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"ORFLAGS",	fun_orflags,	2,  0,		CA_PUBLIC},
{"OWNER",	fun_owner,	1,  0,		CA_PUBLIC},
{"PARENT",	fun_parent,	1,  0,		CA_PUBLIC},
{"PARSE",	fun_parse,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"PEEK",	fun_peek,	0,  FN_VARARGS, CA_PUBLIC},
{"PEMIT",	fun_pemit,	2,  0,		CA_PUBLIC},
{"PFIND",	fun_pfind,	1,  0,		CA_PUBLIC},
{"PI",		fun_pi,		0,  0,		CA_PUBLIC},
{"PLAYMEM",	fun_playmem,	1,  0,		CA_PUBLIC},
{"PMATCH",	fun_pmatch,	1,  0,		CA_PUBLIC},
{"POP",		fun_pop,	0,  FN_VARARGS, CA_PUBLIC},
{"POPN",	fun_popn,	0,  FN_VARARGS,	CA_PUBLIC},
{"PORTS",	fun_ports,	1,  0,		CA_PUBLIC},
{"POS",		fun_pos,	2,  0,		CA_PUBLIC},
{"POSS",	fun_poss,	1,  0,		CA_PUBLIC},
{"POWER",	fun_power,	2,  0,		CA_PUBLIC},
{"PROGRAMMER",	fun_programmer,	1,  0,		CA_PUBLIC},
{"PUSH",	fun_push,	0,  FN_VARARGS, CA_PUBLIC},
{"R",		fun_r,		1,  0,		CA_PUBLIC},
{"RAND",	fun_rand,	1,  0,		CA_PUBLIC},
{"REGMATCH",	fun_regmatch,	0,  FN_VARARGS, CA_PUBLIC},
{"REGPARSE",	fun_regparse,	3,  0,		CA_PUBLIC},
{"REMIT",	fun_remit,	2,  0,		CA_PUBLIC},
{"REMOVE",	fun_remove,	0,  FN_VARARGS,	CA_PUBLIC},
{"REPEAT",	fun_repeat,	2,  0,		CA_PUBLIC},
{"REPLACE",	fun_replace,	0,  FN_VARARGS,	CA_PUBLIC},
{"REST",	fun_rest,	0,  FN_VARARGS,	CA_PUBLIC},
{"RESTARTS",	fun_restarts,	0,  0,		CA_PUBLIC},
{"RESTARTTIME",	fun_restarttime, 0, 0,		CA_PUBLIC},
{"REVERSE",	fun_reverse,	-1, 0,		CA_PUBLIC},
{"REVWORDS",	fun_revwords,	0,  FN_VARARGS,	CA_PUBLIC},
{"RIGHT",	fun_right,	2,  0,		CA_PUBLIC},
{"RJUST",	fun_rjust,	0,  FN_VARARGS,	CA_PUBLIC},
{"RLOC",	fun_rloc,	2,  0,		CA_PUBLIC},
{"ROOM",	fun_room,	1,  0,		CA_PUBLIC},
{"ROUND",	fun_round,	2,  0,		CA_PUBLIC},
{"S",		fun_s,		-1, 0,		CA_PUBLIC},
{"SCRAMBLE",	fun_scramble,	1,  0,		CA_PUBLIC},
{"SEARCH",	fun_search,	-1, 0,		CA_PUBLIC},
{"SECS",	fun_secs,	0,  0,		CA_PUBLIC},
{"SECURE",	fun_secure,	-1, 0,		CA_PUBLIC},
{"SEES",	fun_sees,	2,  0,		CA_PUBLIC},
{"SET",		fun_set,	2,  0,		CA_PUBLIC},
{"SETDIFF",	fun_setdiff,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETINTER",	fun_setinter,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETQ",	fun_setq,	2,  0,		CA_PUBLIC},
{"SETR",	fun_setr,	2,  0,		CA_PUBLIC},
{"SETX",	fun_setx,	2,  0,		CA_PUBLIC},
{"SETUNION",	fun_setunion,	0,  FN_VARARGS,	CA_PUBLIC},
{"SHL",		fun_shl,	2,  0,		CA_PUBLIC},
{"SHR",		fun_shr,	2,  0,		CA_PUBLIC},
{"SHUFFLE",	fun_shuffle,	0,  FN_VARARGS,	CA_PUBLIC},
{"SIGN",	fun_sign,	1,  0,		CA_PUBLIC},
{"SIN",		fun_sin,	1,  0,		CA_PUBLIC},
{"SORT",	fun_sort,	0,  FN_VARARGS,	CA_PUBLIC},
{"SORTBY",	fun_sortby,	0,  FN_VARARGS, CA_PUBLIC},
{"SPACE",	fun_space,	1,  0,		CA_PUBLIC},
{"SPLICE",	fun_splice,	0,  FN_VARARGS,	CA_PUBLIC},
{"SQL",		fun_sql,	0,  FN_VARARGS, CA_SQL_OK},
{"SQRT",	fun_sqrt,	1,  0,		CA_PUBLIC},
{"SQUISH",	fun_squish,	0,  FN_VARARGS,	CA_PUBLIC},
{"STARTTIME",	fun_starttime,	0,  0,		CA_PUBLIC},
{"STATS",	fun_stats,	1,  0,		CA_PUBLIC},
{"STEP",	fun_step,	0,  FN_VARARGS, CA_PUBLIC},
{"STRCAT",	fun_strcat,	0,  FN_VARARGS,	CA_PUBLIC},
{"STREQ",	fun_streq,	2,  0,		CA_PUBLIC},
{"STRIPANSI",	fun_stripansi,	1,  0,		CA_PUBLIC},
{"STRLEN",	fun_strlen,	-1, 0,		CA_PUBLIC},
{"STRMATCH",	fun_strmatch,	2,  0,		CA_PUBLIC},
{"STRTRUNC",    fun_strtrunc,   2,  0,          CA_PUBLIC},
{"STRUCTURE",	fun_structure,	0,  FN_VARARGS,	CA_PUBLIC},
{"SUB",		fun_sub,	2,  0,		CA_PUBLIC},
{"SUBJ",	fun_subj,	1,  0,		CA_PUBLIC},
{"SWAP",	fun_swap,	0,  FN_VARARGS,	CA_PUBLIC},
{"SWITCH",	fun_switch,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"SWITCHALL",	fun_switchall,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"T",		fun_t,		1,  0,		CA_PUBLIC},
{"TAN",		fun_tan,	1,  0,		CA_PUBLIC},
#ifdef TCL_INTERP_SUPPORT
{"TCLCLEAR",	fun_tclclear,	0,  0,		CA_WIZARD},
{"TCLEVAL",	fun_tcleval,	1,  0,		CA_WIZARD},
{"TCLMODULE",	fun_tclmodule,	1,  0,		CA_WIZARD},
{"TCLPARAMS",	fun_tclparams,	0,  FN_VARARGS,	CA_WIZARD},
{"TCLREGS",	fun_tclregs,	0,  0,		CA_WIZARD},
#endif /* TCL_INTERP_SUPPORT */
{"TEL",		fun_tel,	2,  0,		CA_PUBLIC},
{"TIME",	fun_time,	0,  0,		CA_PUBLIC},
{"TOSS",	fun_toss,	0,  FN_VARARGS, CA_PUBLIC},
{"TRANSLATE",	fun_translate,	2,  0,		CA_PUBLIC},
{"TRIGGER",	fun_trigger,	0,  FN_VARARGS, CA_PUBLIC},
{"TRIM",	fun_trim,	0,  FN_VARARGS,	CA_PUBLIC},
{"TRUNC",	fun_trunc,	1,  0,		CA_PUBLIC},
{"TYPE",	fun_type,	1,  0,		CA_PUBLIC},
{"U",		fun_u,		0,  FN_VARARGS,	CA_PUBLIC},
{"UCSTR",	fun_ucstr,	-1, 0,		CA_PUBLIC},
{"UDEFAULT",	fun_udefault,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"ULOCAL",	fun_ulocal,	0,  FN_VARARGS,	CA_PUBLIC},
{"UNLOAD",	fun_unload,	0,  FN_VARARGS,	CA_PUBLIC},
{"UNSTRUCTURE",	fun_unstructure,1,  0,		CA_PUBLIC},
#ifdef PUEBLO_SUPPORT
{"URL_ESCAPE",	fun_url_escape,	-1, 0,		CA_PUBLIC},
{"URL_UNESCAPE",fun_url_unescape,-1,0,		CA_PUBLIC},
#endif /* PUEBLO_SUPPORT */
{"V",		fun_v,		1,  0,		CA_PUBLIC},
{"VADD",	fun_vadd,	0,  FN_VARARGS,	CA_PUBLIC},
{"VALID",	fun_valid,	2,  FN_VARARGS, CA_PUBLIC},
{"VDIM",	fun_vdim,	0,  FN_VARARGS,	CA_PUBLIC},
{"VDOT",	fun_vdot,	0,  FN_VARARGS,	CA_PUBLIC},
{"VERSION",	fun_version,	0,  0,		CA_PUBLIC},
{"VISIBLE",	fun_visible,	2,  0,		CA_PUBLIC},
{"VMAG",	fun_vmag,	0,  FN_VARARGS,	CA_PUBLIC},
{"VMUL",	fun_vmul,	0,  FN_VARARGS,	CA_PUBLIC},
{"VSUB",	fun_vsub,	0,  FN_VARARGS,	CA_PUBLIC},
{"VUNIT",	fun_vunit,	0,  FN_VARARGS,	CA_PUBLIC},
{"WAIT",	fun_wait,	2,  0,		CA_PUBLIC},
{"WHERE",	fun_where,	1,  0,		CA_PUBLIC},
{"WHILE",	fun_while,	0,  FN_VARARGS,	CA_PUBLIC},
{"WIPE",	fun_wipe,	1,  0,		CA_PUBLIC},
{"WORDPOS",     fun_wordpos,    0,  FN_VARARGS,	CA_PUBLIC},
{"WORDS",	fun_words,	0,  FN_VARARGS,	CA_PUBLIC},
{"X",		fun_x,		1,  0,		CA_PUBLIC},
{"XCON",	fun_xcon,	3,  0,		CA_PUBLIC},
{"XGET",	fun_xget,	2,  0,		CA_PUBLIC},
{"XOR",		fun_xor,	0,  FN_VARARGS,	CA_PUBLIC},
{"XORBOOL",	fun_xorbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"XVARS",	fun_xvars,	0,  FN_VARARGS, CA_PUBLIC},
{"Z",		fun_z,		2,  0,		CA_PUBLIC},
{"ZFUN",	fun_zfun,	0,  FN_VARARGS,	CA_PUBLIC},
{"ZONE",        fun_zone,       1,  0,          CA_PUBLIC},
{"ZWHO",        fun_zwho,       1,  0,          CA_PUBLIC},
{NULL,		NULL,		0,  0,		0}};

/* *INDENT-ON* */

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
		ufp->name = strsave(np);
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
