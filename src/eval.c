/* eval.c - command evaluation and cracking */
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

#include "powers.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "ansi.h"	/* required by code */

extern char qidx_chartab[256];	/* from funvars.c */

/* ---------------------------------------------------------------------------
 * parse_to: Split a line at a character, obeying nesting.  The line is
 * destructively modified (a null is inserted where the delimiter was found)
 * dstr is modified to point to the char after the delimiter, and the function
 * return value points to the found string (space compressed if specified).
 * If we ran off the end of the string without finding the delimiter, dstr is
 * returned as NULL.
 */

static char *parse_to_cleanup(eval, first, cstr, rstr, zstr)
int eval, first;
char *cstr, *rstr, *zstr;
{
	if ((mudconf.space_compress || (eval & EV_STRIP_TS)) &&
	    !(eval & EV_NO_COMPRESS) && !first && (cstr[-1] == ' '))
		zstr--;
	if ((eval & EV_STRIP_AROUND) && (*rstr == '{') && (zstr[-1] == '}')) {
		rstr++;
		if ((mudconf.space_compress && !(eval & EV_NO_COMPRESS)) ||
		    (eval & EV_STRIP_LS))
			while (*rstr && isspace(*rstr))
				rstr++;
		rstr[-1] = '\0';
		zstr--;
		if ((mudconf.space_compress && !(eval & EV_NO_COMPRESS)) ||
		    (eval & EV_STRIP_TS))
			while (zstr[-1] && isspace(zstr[-1]))
				zstr--;
		*zstr = '\0';
	}
	*zstr = '\0';
	return rstr;
}

/* We can't change this to just '*zstr++ = *cstr++', because of the inherent
problems with copying a memory location to itself. */

#define NEXTCHAR \
	if (cstr == zstr) { \
		cstr++; \
		zstr++; \
	} else \
		*zstr++ = *cstr++


char *parse_to(dstr, delim, eval)
char **dstr, delim;
int eval;
{
#define stacklim 32
	char stack[stacklim];
	char *rstr, *cstr, *zstr;
	int sp, tp, first, bracketlev;

	if ((dstr == NULL) || (*dstr == NULL))
		return NULL;
	if (**dstr == '\0') {
		rstr = *dstr;
		*dstr = NULL;
		return rstr;
	}
	sp = 0;
	first = 1;
	rstr = *dstr;
	if ((mudconf.space_compress || (eval & EV_STRIP_LS)) &&
	    !(eval & EV_NO_COMPRESS)) {
		while (*rstr && isspace(*rstr))
			rstr++;
		*dstr = rstr;
	}
	zstr = cstr = rstr;
	while (*cstr) {
		switch (*cstr) {
		case '\\':	/* general escape */
		case '%':	/* also escapes chars */
		        if ((*cstr == '\\') && (eval & EV_STRIP_ESC)) {
				cstr++;
			} else {
				NEXTCHAR;
			}
			if (*cstr) {
				NEXTCHAR;
			}
			first = 0;
			break;
		case ']':
		case ')':
			for (tp = sp - 1; (tp >= 0) && (stack[tp] != *cstr); tp--) ;

			/* If we hit something on the stack, unwind to it 
			 * Otherwise (it's not on stack), if it's our
			 * delim  we are done, and we convert the 
			 * delim to a null and return a ptr to the
			 * char after the null. If it's not our
			 * delimiter, skip over it normally  
			 */

			if (tp >= 0)
				sp = tp;
			else if (*cstr == delim) {
				rstr = parse_to_cleanup(eval, first,
							cstr, rstr, zstr);
				*dstr = ++cstr;
				return rstr;
			}
			first = 0;
			NEXTCHAR;
			break;
		case '{':
			bracketlev = 1;
			if (eval & EV_STRIP) {
				cstr++;
			} else {
				NEXTCHAR;
			}
			while (*cstr && (bracketlev > 0)) {
				switch (*cstr) {
				case '\\':
				case '%':
					if (cstr[1]) {
						if ((*cstr == '\\') &&
						    (eval & EV_STRIP_ESC))
							cstr++;
						else
							NEXTCHAR;
					}
					break;
				case '{':
					bracketlev++;
					break;
				case '}':
					bracketlev--;
					break;
				}
				if (bracketlev > 0) {
					NEXTCHAR;
				}
			}
			if ((eval & EV_STRIP) && (bracketlev == 0)) {
				cstr++;
			} else if (bracketlev == 0) {
				NEXTCHAR;
			}
			first = 0;
			break;
		default:
			if ((*cstr == delim) && (sp == 0)) {
				rstr = parse_to_cleanup(eval, first,
							cstr, rstr, zstr);
				*dstr = ++cstr;
				return rstr;
			}
			switch (*cstr) {
			case ' ':	/* space */
				if (mudconf.space_compress &&
				    !(eval & EV_NO_COMPRESS)) {
					if (first)
						rstr++;
					else if (cstr[-1] == ' ')
						zstr--;
				}
				break;
			case '[':
				if (cstr[-1] == ESC_CHAR) {
					first = 0;
					break;
				}
				if (sp < stacklim)
					stack[sp++] = ']';
				first = 0;
				break;
			case '(':
				if (sp < stacklim)
					stack[sp++] = ')';
				first = 0;
				break;
			default:
				first = 0;
			}
			NEXTCHAR;
		}
	}
	rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
	*dstr = NULL;
	return rstr;
}

/* ---------------------------------------------------------------------------
 * parse_arglist: Parse a line into an argument list contained in lbufs.
 * A pointer is returned to whatever follows the final delimiter.
 * If the arglist is unterminated, a NULL is returned.  The original arglist 
 * is destructively modified.
 */

char *parse_arglist(player, cause, dstr, delim, eval,
		    fargs, nfargs, cargs, ncargs)
dbref player, cause, eval, nfargs, ncargs;
char *dstr, delim, *fargs[], *cargs[];
{
	char *rstr, *tstr, *bp, *str;
	int arg, peval;

	for (arg = 0; arg < nfargs; arg++)
		fargs[arg] = NULL;
	if (dstr == NULL)
		return NULL;
	rstr = parse_to(&dstr, delim, 0);
	arg = 0;

	peval = (eval & ~EV_EVAL);

	while ((arg < nfargs) && rstr) {
		if (arg < (nfargs - 1))
			tstr = parse_to(&rstr, ',', peval);
		else
			tstr = parse_to(&rstr, '\0', peval);
		if (eval & EV_EVAL) {
			bp = fargs[arg] = alloc_lbuf("parse_arglist");
			str = tstr;
			exec(fargs[arg], &bp, 0, player, cause, eval | EV_FCHECK, &str,
			     cargs, ncargs);
			*bp = '\0';
		} else {
			fargs[arg] = alloc_lbuf("parse_arglist");
			strcpy(fargs[arg], tstr);
		}
		arg++;
	}
	return dstr;
}

/* ---------------------------------------------------------------------------
 * exec: Process a command line, evaluating function calls and %-substitutions.
 */

int get_gender(player)
dbref player;
{
	char first, *atr_gotten;
	dbref aowner;
	int aflags, alen;

	atr_gotten = atr_pget(player, A_SEX, &aowner, &aflags, &alen);
	first = *atr_gotten;
	free_lbuf(atr_gotten);
	switch (first) {
	case 'P':
	case 'p':
		return 4;
	case 'M':
	case 'm':
		return 3;
	case 'F':
	case 'f':
	case 'W':
	case 'w':
		return 2;
	default:
		return 1;
	}
}

/* ---------------------------------------------------------------------------
 * Trace cache routines.
 */

typedef struct tcache_ent TCENT;
struct tcache_ent {
	char *orig;
	char *result;
	struct tcache_ent *next;
} *tcache_head;
int tcache_top, tcache_count;

void NDECL(tcache_init)
{
	tcache_head = NULL;
	tcache_top = 1;
	tcache_count = 0;
}

int NDECL(tcache_empty)
{
	if (tcache_top) {
		tcache_top = 0;
		tcache_count = 0;
		return 1;
	}
	return 0;
}

static void tcache_add(orig, result)
char *orig, *result;
{
	char *tp;
	TCENT *xp;

	if (strcmp(orig, result)) {
		tcache_count++;
		if (tcache_count <= mudconf.trace_limit) {
			xp = (TCENT *) alloc_sbuf("tcache_add.sbuf");
			tp = alloc_lbuf("tcache_add.lbuf");
			strcpy(tp, result);
			xp->orig = orig;
			xp->result = tp;
			xp->next = tcache_head;
			tcache_head = xp;
		} else {
			free_lbuf(orig);
		}
	} else {
		free_lbuf(orig);
	}
}

static void tcache_finish(player)
dbref player;
{
	TCENT *xp;
	NUMBERTAB *np;
	dbref target;

	if (H_Redirect(player)) {
	    np = (NUMBERTAB *) nhashfind(player, &mudstate.redir_htab);
	    if (np) {
		target = np->num; 
	    } else {
		/* Ick. If we have no pointer, we should have no flag. */
		s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
		target = Owner(player);
	    }
	} else {
	    target = Owner(player);
	}

	while (tcache_head != NULL) {
		xp = tcache_head;
		tcache_head = xp->next;
		notify(target,
		       tprintf("%s(#%d)} '%s' -> '%s'", Name(player), player,
			       xp->orig, xp->result));
		free_lbuf(xp->orig);
		free_lbuf(xp->result);
		free_sbuf(xp);
	}
	tcache_top = 1;
	tcache_count = 0;
}

/* Character table for fast lookups.
 * 0  - 31  (00 - 1F): NULL plus other specials
 * 32 - 63  (20 - 3F): space through ? (symbol characters)
 * 64 - 95  (40 - 5F): @ through _ (includes upper-case letters)
 * 96 - 127 (60 - 7F): ` through delete (includes lower-case letters)
 *
 * We want ' ', '\', '[', '{', '(', '%', and '#'.
 */

char special_chartab[256] =
{
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,0,0,1,0,1,0,0, 1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,1,1,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,1,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};

char token_chartab[256] =
{
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,1,0,1,1,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};

char *ansi_chartab[256] =
{
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,           0,             ANSI_BBLUE,  ANSI_BCYAN,
    0,           0,             0,           ANSI_BGREEN,
    0,           0,             0,           0,
    0,           ANSI_BMAGENTA, 0,           0,
    0,           0,             ANSI_BRED,   0,
    0,           0,             0,           ANSI_BWHITE,
    ANSI_BBLACK, ANSI_BYELLOW,  0,           0,
    0,           0,             0,           0,
    0,           0,             ANSI_BLUE,   ANSI_CYAN,
    0,           0,             ANSI_BLINK,  ANSI_GREEN,
    ANSI_HILITE, ANSI_INVERSE,  0,           0,
    0,           ANSI_MAGENTA,  ANSI_NORMAL, 0,
    0,           0,             ANSI_RED,    0,
    0,           ANSI_UNDER,    0,           ANSI_WHITE,
    ANSI_BLACK,  ANSI_YELLOW,   0,           0,
    0,           0,             0,           0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};

void exec(buff, bufc, tflags, player, cause, eval, dstr, cargs, ncargs)
char *buff, **bufc;
int tflags;
dbref player, cause;
int eval, ncargs;
char **dstr;
char *cargs[];
{
#define	NFARGS	30
	char *fargs[NFARGS], *preserve[MAX_GLOBAL_REGS];
	char *tstr, *tbuf, *savepos, *atr_gotten, *start, *oldp;
	char savec, ch, *savestr, *str, *xptr, *mundane, *p;
	char *realbuff = NULL, *realbp = NULL;
	char xtbuf[SBUF_SIZE], *xtp;
	dbref aowner;
	int at_space, nfargs, gender, i, j, alldone, aflags, alen, feval;
	int is_trace, is_top, save_count, preserve_len[MAX_GLOBAL_REGS];
	int ansi, nchar, navail;
	FUN *fp;
	UFUN *ufp;
	VARENT *xvar;
	ATTR *ap;

	static const char *subj[5] =
	{"", "it", "she", "he", "they"};
	static const char *poss[5] =
	{"", "its", "her", "his", "their"};
	static const char *obj[5] =
	{"", "it", "her", "him", "them"};
	static const char *absp[5] =
	{"", "its", "hers", "his", "theirs"};


	if (*dstr == NULL)
		return;

	at_space = 1;
	gender = -1;
	alldone = 0;
	ansi = 0;

	is_trace = Trace(player) && !(eval & EV_NOTRACE);
	is_top = 0;

	/* Extend the buffer if we need to. */
	
	if (((*bufc) - buff) > (LBUF_SIZE - SBUF_SIZE)) {
		realbuff = buff;
		realbp = *bufc;
		buff = (char *)XMALLOC(LBUF_SIZE, "exec.buff_extend");
		*bufc = buff;
	}

	oldp = start = *bufc;
	
	/* If we are tracing, save a copy of the starting buffer */

	savestr = NULL;
	if (is_trace) {
		is_top = tcache_empty();
		savestr = alloc_lbuf("exec.save");
		strcpy(savestr, *dstr);
	}
	while (**dstr && !alldone) {

	    /* We adjust the special table every time we go around this
	     * loop, in order to avoid always treating '#' like a special
	     * character, as it gets used a whole heck of a lot.
	     */
	    special_chartab[(unsigned char) '#'] =
		(mudstate.in_loop || mudstate.in_switch) ? 1 : 0;

	    if (!special_chartab[(unsigned char) **dstr]) {
		/* Mundane characters are the most common. There are usually
		 * a bunch in a row. We should just copy them.
		 */
		mundane = *dstr;
		nchar = 0;
		do {
		    nchar++;
		} while (!special_chartab[(unsigned char) *(++mundane)]);
		p = *bufc;
		navail = LBUF_SIZE - 1 - (p - buff);
		nchar = (nchar > navail) ? navail : nchar;
		memcpy(p, *dstr, nchar);
		*bufc = p + nchar;
		*dstr = mundane;
		at_space = 0;
	    }

	    /* We must have a special character at this point. */

	    if (**dstr == '\0')
		break;

	    switch (**dstr) {
		case ' ':
			/* A space.  Add a space if not compressing or if
			 * previous char was not a space 
			 */

			if (!(mudconf.space_compress && at_space) ||
			    (eval & EV_NO_COMPRESS)) {
				safe_chr(' ', buff, bufc);
				at_space = 1;
			}
			break;
		case '\\':
			/* General escape.  Add the following char without
			 * special processing 
			 */

			at_space = 0;
			(*dstr)++;
			if (**dstr) {
				safe_chr(**dstr, buff, bufc);
			} else
				(*dstr)--;
			break;
		case '[':
			/* Function start.  Evaluate the contents of the 
			 * square brackets as a function.  If no closing
			 * bracket, insert the [ and continue. 
			 */

			at_space = 0;
			tstr = (*dstr)++;
			if (eval & EV_NOFCHECK) {
				safe_chr('[', buff, bufc);
				*dstr = tstr;
				break;
			}
			tbuf = parse_to(dstr, ']', 0);
			if (*dstr == NULL) {
				safe_chr('[', buff, bufc);
				*dstr = tstr;
			} else {
				str = tbuf;
				exec(buff, bufc, 0, player, cause,
				     (eval | EV_FCHECK | EV_FMAND),
				     &str, cargs, ncargs);
				(*dstr)--;
			}
			break;
		case '{':
			/* Literal start.  Insert everything up to the
			 * terminating } without parsing.  If no closing
			 * brace, insert the { and continue. 
			 */

			at_space = 0;
			tstr = (*dstr)++;
			tbuf = parse_to(dstr, '}', 0);
			if (*dstr == NULL) {
				safe_chr('{', buff, bufc);
				*dstr = tstr;
			} else {
				if (!(eval & EV_STRIP)) {
					safe_chr('{', buff, bufc);
				}
				/* Preserve leading spaces (Felan) */

				if (*tbuf == ' ') {
					safe_chr(' ', buff, bufc);
					tbuf++;
				}
				str = tbuf;
				exec(buff, bufc, 0, player, cause,
				     (eval & ~(EV_STRIP | EV_FCHECK)),
				     &str, cargs, ncargs);
				if (!(eval & EV_STRIP)) {
					safe_chr('}', buff, bufc);
				}
				(*dstr)--;
			}
			break;
		case '%':
			/* Percent-replace start.  Evaluate the chars
			 * following and perform the appropriate
			 * substitution. 
			 */

			at_space = 0;
			(*dstr)++;
			savec = **dstr;
			savepos = *bufc;
			switch (savec) {
			case '\0':	/* Null - all done */
				(*dstr)--;
				break;
			case '|':	/* piped command output */
				safe_str(mudstate.pout, buff, bufc);
				break;
			case '%':	/* Percent - a literal % */
				safe_chr('%', buff, bufc);
				break;
			case 'x':	/* ANSI color */
			case 'X':
				(*dstr)++;
				if (!**dstr) {
				    /*
				     * Note: There is an interesting
				     * bug/misfeature in the implementation
				     * of %v? and %q? -- if the second
				     * character is garbage or non-existent,
				     * it and the leading v or q gets eaten.
				     * In the interests of not changing the
				     * old behavior, this is not getting
				     * "fixed", but in this case, where
				     * moving the pointer back without
				     * exiting on an error condition ends up
				     * turning things black, the behavior
				     * must by necessity be different. So we
				     * do  break out of the switch.
				     */
				    (*dstr)--;
				    break;
				}
				if (!mudconf.ansi_colors) {
				    /* just skip over the characters */
				    break;
				}
				if (!ansi_chartab[(unsigned char) **dstr]) {
				    safe_chr(**dstr, buff, bufc);
				} else {
				    safe_str(ansi_chartab[(unsigned char) **dstr], buff, bufc);
				    ansi = (**dstr == 'n') ? 0 : 1;
				}
				break;
			case 'r':	/* Carriage return */
			case 'R':
				safe_crlf(buff, bufc);
				break;
			case 't':	/* Tab */
			case 'T':
				safe_chr('\t', buff, bufc);
				break;
			case 'B':	/* Blank */
			case 'b':
				safe_chr(' ', buff, bufc);
				break;
			case '0':	/* Command argument number N */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i = (**dstr - '0');
				if ((i < ncargs) && (cargs[i] != NULL))
					safe_str(cargs[i], buff, bufc);
				break;
			case '=': /* equivalent of generic v() attr get */
			        (*dstr)++;
				if (**dstr != '<') {
				    (*dstr)--;
				    break;
				}
				xptr = *dstr;
				(*dstr)++;
				if (!**dstr) {
				    *dstr = xptr;
				    break;
				}
				xtp = xtbuf;
				while (**dstr && (**dstr != '>')) {
				    safe_sb_chr(**dstr, xtbuf, &xtp);
				    (*dstr)++;
				}
				if (**dstr != '>') {
				    /* Ran off the end. Back up. */
				    *dstr = xptr;
				    break;
				}
				*xtp = '\0';
				ap = atr_str(xtbuf);
				if (!ap)
				    break;
				atr_pget_info(player, ap->number, 
					      &aowner, &aflags);
				if (See_attr(player, player, ap,
					     aowner, aflags)) {
				    atr_gotten = atr_pget(player, ap->number,
							  &aowner, &aflags,
							  &alen);
				    safe_known_str(atr_gotten, alen,
						   buff, bufc);
				    free_lbuf(atr_gotten);
				}
				break;
			case '_':       /* x-variable */
			        (*dstr)++;
				/* Check for %_<varname> */
				if (**dstr != '<') {
				    ch = tolower(**dstr);
				    if (!**dstr)
					(*dstr)--;
				    if (!isalnum(ch))
					break;
				    xtp = xtbuf;
				    safe_ltos(xtbuf, &xtp, player);
				    safe_chr('.', xtbuf, &xtp);
				    safe_chr(ch, xtbuf, &xtp);
				} else {
				    xptr = *dstr;
				    (*dstr)++;
				    if (!**dstr) {
					*dstr = xptr;
					break;
				    }
				    xtp = xtbuf;
				    safe_ltos(xtbuf, &xtp, player);
				    safe_chr('.', xtbuf, &xtp);
				    while (**dstr && (**dstr != '>')) {
					/* Copy. No interpretation. */
					ch = tolower(**dstr);
					safe_sb_chr(ch, xtbuf, &xtp);
					(*dstr)++;
				    }
				    if (**dstr != '>') {
					/* We ran off the end of the string
					 * without finding a termination
					 * condition. Go back.
					 */
					*dstr = xptr;
					break;
				    }
				}
				*xtp = '\0';
				if ((xvar = (VARENT *) hashfind(xtbuf,
						       &mudstate.vars_htab))) {
				    safe_str(xvar->text, buff, bufc);
				}
				break;
			case 'V':	/* Variable attribute */
			case 'v':
				(*dstr)++;
				ch = toupper(**dstr);
				if (!**dstr)
					(*dstr)--;
				if ((ch < 'A') || (ch > 'Z'))
					break;
				i = 100 + ch - 'A';
				atr_gotten = atr_pget(player, i, &aowner,
						      &aflags, &alen);
				safe_known_str(atr_gotten, alen, buff, bufc);
				free_lbuf(atr_gotten);
				break;
			case 'Q':	/* Local registers */
			case 'q':
				(*dstr)++;
				if (!**dstr)
					(*dstr)--;
				i = qidx_chartab[(unsigned char) **dstr];
				if ((i < 0) || (i >= MAX_GLOBAL_REGS))
					break;
				if (mudstate.global_regs[i]) {
					safe_known_str(mudstate.global_regs[i],
						      mudstate.glob_reg_len[i],
						       buff, bufc);
				}
				if (!**dstr)
					(*dstr)--;
				break;
			case 'O':	/* Objective pronoun */
			case 'o':
				if (gender < 0)
					gender = get_gender(cause);
				if (!gender)
					safe_name(cause, buff, bufc);
				else
					safe_str((char *)obj[gender],
						 buff, bufc);
				break;
			case 'P':	/* Personal pronoun */
			case 'p':
				if (gender < 0)
					gender = get_gender(cause);
				if (!gender) {
				        safe_name(cause, buff, bufc);
					safe_chr('s', buff, bufc);
				} else {
					safe_str((char *)poss[gender],
						 buff, bufc);
				}
				break;
			case 'S':	/* Subjective pronoun */
			case 's':
				if (gender < 0)
					gender = get_gender(cause);
				if (!gender)
					safe_name(cause, buff, bufc);
				else
					safe_str((char *)subj[gender],
						 buff, bufc);
				break;
			case 'A':	/* Absolute posessive */
			case 'a':	/* idea from Empedocles */
				if (gender < 0)
					gender = get_gender(cause);
				if (!gender) {
				        safe_name(cause, buff, bufc);
					safe_chr('s', buff, bufc);
				} else {
					safe_str((char *)absp[gender],
						 buff, bufc);
				}
				break;
			case '#':	/* Invoker DB number */
				safe_dbref(buff, bufc, cause);
				break;
			case '!':	/* Executor DB number */
				safe_dbref(buff, bufc, player);
				break;
			case 'N':	/* Invoker name */
			case 'n':
				safe_name(cause, buff, bufc);
				break;
			case 'L':	/* Invoker location db# */
			case 'l':
				if (!(eval & EV_NO_LOCATION)) {
					safe_dbref(buff, bufc, where_is(cause));
				}
				break;
			case 'C':
			case 'c':
				safe_str(mudstate.curr_cmd, buff, bufc);
				break;
			default:	/* Just copy */
				safe_chr(**dstr, buff, bufc);
			}
			if (isupper(savec))
				*savepos = toupper(*savepos);
			break;
		case '(':
			/* Arglist start.  See if what precedes is a
			 * function. If so, execute it if we should. 
			 */

			at_space = 0;
			if (!(eval & EV_FCHECK)) {
				safe_chr('(', buff, bufc);
				break;
			}
			/* Load an sbuf with an uppercase version of the func
			 * name, and see if the func exists.  Trim 
			 * trailing spaces from the name if configured. 
			 */

			**bufc = '\0';
			xtp = xtbuf;
			safe_sb_str(oldp, xtbuf, &xtp);
			*xtp = '\0';
			if (mudconf.space_compress && (eval & EV_FMAND)) {
				while ((--xtp >= xtbuf) && isspace(*xtp)) ;
				xtp++;
				*xtp = '\0';
			}
			for (xtp = xtbuf; *xtp; xtp++)
				*xtp = tolower(*xtp);
			fp = (FUN *) hashfind(xtbuf, &mudstate.func_htab);

			/* If not a builtin func, check for global func */

			ufp = NULL;
			if (fp == NULL) {
				ufp = (UFUN *) hashfind(xtbuf,
							&mudstate.ufunc_htab);
			}
			/* Do the right thing if it doesn't exist */

			if (!fp && !ufp) {
				if (eval & EV_FMAND) {
					*bufc = oldp;
					safe_tprintf_str(buff, bufc, "#-1 FUNCTION (%s) NOT FOUND", xtbuf);
					alldone = 1;
				} else {
					safe_chr('(', buff, bufc);
				}
				eval &= ~EV_FCHECK;
				break;
			}

			/* Get the arglist and count the number of args Neg 
			 * # of args means catenate subsequent args 
			 */

			if (ufp)
				nfargs = NFARGS;
			else if (fp->nargs < 0)
				nfargs = -fp->nargs;
			else
				nfargs = NFARGS;
			tstr = *dstr;
			if ((fp && (fp->flags & FN_NO_EVAL)) ||
			    (ufp && (ufp->flags & FN_NO_EVAL)))
				feval = (eval & ~EV_EVAL) | EV_STRIP_ESC;
			else
				feval = eval;
			*dstr = parse_arglist(player, cause, *dstr + 1,
					      ')', feval, fargs, nfargs,
					      cargs, ncargs);

			/* If no closing delim, just insert the '(' and
			 * continue normally 
			 */

			if (!*dstr) {
				*dstr = tstr;
				safe_chr(**dstr, buff, bufc);
				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
						free_lbuf(fargs[i]);
				eval &= ~EV_FCHECK;
				break;
			}
			/* Count number of args returned */

			(*dstr)--;
			j = 0;
			for (i = 0; i < nfargs; i++)
				if (fargs[i] != NULL)
					j = i + 1;
			nfargs = j;

			/* We've got function(args) now, so back up over
			 * function name in output buffer
			 */
			*bufc = oldp;

			/* If it's a user-defined function, perform it now. */

			if (ufp) {
				mudstate.func_nest_lev++;
				mudstate.func_invk_ctr++;
				if (mudstate.func_nest_lev >=
				    mudconf.func_nest_lim) {
					safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				} else if (mudstate.func_invk_ctr >=
					   mudconf.func_invk_lim) {
					safe_str("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				} else if (Going(player)) {
				        safe_str("#-1 BAD INVOKER", buff, bufc);
				} else if (!check_access(player, ufp->perms)) {
					safe_noperm(buff, bufc);
				} else {
					tstr = atr_get(ufp->obj, ufp->atr,
						       &aowner, &aflags,
						       &alen);
					if (ufp->flags & FN_PRIV)
						i = ufp->obj;
					else
						i = player;
					str = tstr;
					
					if (ufp->flags & FN_PRES) {
					    save_global_regs("eval_save",
							     preserve,
							     preserve_len);
					}
					
					exec(buff, bufc, 0, i, cause,
					     ((ufp->flags & FN_NO_EVAL) ?
					      (EV_FCHECK | EV_EVAL) : feval),
					     &str, fargs, nfargs);
					
					if (ufp->flags & FN_PRES) {
					    restore_global_regs("eval_restore",
								preserve,
								preserve_len);
					}

					free_lbuf(tstr);
				}

				/* Return the space allocated for the args */

				mudstate.func_nest_lev--;
				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
						free_lbuf(fargs[i]);
				eval &= ~EV_FCHECK;
				break;
			}
			/* If the number of args is right, perform the func.
			 * Otherwise return an error message.  Note
			 * that parse_arglist returns zero args as one
			 * null arg, so we have to handle that case
			 * specially. 
			 */

			if ((fp->nargs == 0) && (nfargs == 1)) {
				if (!*fargs[0]) {
					free_lbuf(fargs[0]);
					fargs[0] = NULL;
					nfargs = 0;
				}
			}
			if ((nfargs == fp->nargs) ||
			    (nfargs == -fp->nargs) ||
			    (fp->flags & FN_VARARGS)) {

				/* Check recursion limit */

				mudstate.func_nest_lev++;
				mudstate.func_invk_ctr++;
				if (mudstate.func_nest_lev >=
				    mudconf.func_nest_lim) {
					safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				} else if (mudstate.func_invk_ctr >=
					   mudconf.func_invk_lim) {
					safe_str("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				} else if (Going(player)) {
					/* Deal with the peculiar case of the
					 * calling object being destroyed
					 * mid-function sequence, such as
					 * with a command()/@destroy combo...
					 */
					safe_str("#-1 BAD INVOKER", buff, bufc);
				} else if (!check_access(player, fp->perms)) {
					safe_noperm(buff, bufc);
				} else {
					fp->fun(buff, bufc, player, cause,
					      fargs, nfargs, cargs, ncargs);
				}
				mudstate.func_nest_lev--;
			} else {
			  safe_tprintf_str(buff, bufc,
					   "#-1 FUNCTION (%s) EXPECTS %d ARGUMENTS",
					   fp->name, fp->nargs);
			}

			/* Return the space allocated for the arguments */

			for (i = 0; i < nfargs; i++)
				if (fargs[i] != NULL)
					free_lbuf(fargs[i]);
			eval &= ~EV_FCHECK;
			break;
		case '#':
		        /* We should never reach this point unless we're
			 * in a loop or switch, thanks to the table lookup.
			 */

		        at_space = 0;
			(*dstr)++;
			if (!token_chartab[(unsigned char) **dstr]) {
			    (*dstr)--;
			    safe_chr(**dstr, buff, bufc);
			} else {
			    if ((**dstr == '#') && mudstate.in_loop) {
			      safe_str(mudstate.loop_token[mudstate.in_loop-1],
				       buff, bufc);
			    } else if ((**dstr == '@') && mudstate.in_loop) {
			      safe_ltos(buff, bufc,
				     mudstate.loop_number[mudstate.in_loop-1]);
			    } else if ((**dstr == '$') && mudstate.in_switch) {
				safe_str(mudstate.switch_token, buff, bufc);
			    } else if (**dstr == '!') {
				/* Nesting level of loop takes precedence
				 * over switch nesting level.
				 */
				safe_ltos(buff, bufc,
					  ((mudstate.in_loop) ?
					   (mudstate.in_loop - 1):
					   mudstate.in_switch));
			    } else {
				(*dstr)--;
				safe_chr(**dstr, buff, bufc);
			    }
			}
			break;
	    }
	    (*dstr)++;
	}

	/* If we're eating spaces, and the last thing was a space, eat it
	 * up. Complicated by the fact that at_space is initially
	 * true. So check to see if we actually put something in the
	 * buffer, too. 
	 */

	if (mudconf.space_compress && at_space && !(eval & EV_NO_COMPRESS)
	    && (start != *bufc))
		(*bufc)--;

	/* The ansi() function knows how to take care of itself. However, 
	 * if the player used a %x sub in the string, and hasn't yet
	 * terminated the color with a %xn yet, we'll have to do it for 
	 * them. 
	 */

	if (ansi)
		safe_ansi_normal(buff, bufc);

	**bufc = '\0';

	if (realbuff) {
		*bufc = realbp;
		safe_str(buff, realbuff, bufc);
		**bufc = '\0';
		XFREE(buff, "exec.buff_extend");
		buff = realbuff;
	}
	
	/* Report trace information */

	if (is_trace) {
		tcache_add(savestr, start);
		save_count = tcache_count - mudconf.trace_limit;;
		if (is_top || !mudconf.trace_topdown)
			tcache_finish(player);
		if (is_top && (save_count > 0)) {
			tbuf = alloc_mbuf("exec.trace_diag");
			sprintf(tbuf,
				"%d lines of trace output discarded.",
				save_count);
			notify(player, tbuf);
			free_mbuf(tbuf);
		}
	}
}

/* ---------------------------------------------------------------------------
 * save_global_regs, restore_global_regs:  Save and restore the global
 * registers to protect them from various sorts of munging.
 */

void save_global_regs(funcname, preserve, preserve_len)
    const char *funcname;
    char *preserve[];
    int preserve_len[];
{
    int i;

    for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	if (!mudstate.global_regs[i]) {
	    preserve[i] = NULL;
	    preserve_len[i] = 0;
	} else {
	    preserve[i] = alloc_lbuf(funcname);
	    memcpy(preserve[i], mudstate.global_regs[i],
		   mudstate.glob_reg_len[i] + 1);
	    preserve_len[i] = mudstate.glob_reg_len[i];
	}
    }
}

void restore_global_regs(funcname, preserve, preserve_len)
    const char *funcname;
    char *preserve[];
    int preserve_len[];
{
    int i;

    for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	if (preserve[i]) {
	    if (!mudstate.global_regs[i])
		mudstate.global_regs[i] = alloc_lbuf(funcname);
	    memcpy(mudstate.global_regs[i], preserve[i],
		   preserve_len[i] + 1);
	    free_lbuf(preserve[i]);
	    mudstate.glob_reg_len[i] = preserve_len[i];
	} else {
	    if (mudstate.global_regs[i])
		*(mudstate.global_regs[i]) = '\0';
	    mudstate.glob_reg_len[i] = 0;
	}
    }
}
