/* boolexp.c */
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

#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "ansi.h"	/* required by code */

static int parsing_internal = 0;

/* ---------------------------------------------------------------------------
 * check_attr: indicate if attribute ATTR on player passes key when checked by
 * the object lockobj
 */

static int check_attr(player, lockobj, attr, key)
dbref player, lockobj;
ATTR *attr;
char *key;
{
	char *buff;
	dbref aowner;
	int aflags, alen, checkit;

	buff = atr_pget(player, attr->number, &aowner, &aflags, &alen);
	checkit = 0;

	if (attr->number == A_LCONTROL) {
		/* We can see control locks... else we'd break zones */
		checkit = 1;
	} else if (See_attr(lockobj, player, attr, aowner, aflags)) {
		checkit = 1;
	} else if (attr->number == A_NAME) {
		checkit = 1;
	}
	if (checkit && (!wild_match(key, buff))) {
		checkit = 0;
	}
	free_lbuf(buff);
	return checkit;
}

static dbref lock_originator = NOTHING;	/* grotesque hack */

int eval_boolexp(player, thing, from, b)
dbref player, thing, from;
BOOLEXP *b;
{
	dbref aowner, obj, source;
	int aflags, alen, c, checkit;
	char *key, *buff, *buff2, *bp, *str;
	ATTR *a;
	GDATA *preserve;

	if (b == TRUE_BOOLEXP)
		return 1;

	switch (b->type) {
	case BOOLEXP_AND:
		return (eval_boolexp(player, thing, from, b->sub1) &&
			eval_boolexp(player, thing, from, b->sub2));
	case BOOLEXP_OR:
		return (eval_boolexp(player, thing, from, b->sub1) ||
			eval_boolexp(player, thing, from, b->sub2));
	case BOOLEXP_NOT:
		return !eval_boolexp(player, thing, from, b->sub1);
	case BOOLEXP_INDIR:
		/*
		 * BOOLEXP_INDIR (i.e. @) is a unary operation which is replaced at
		 * evaluation time by the lock of the object whose number is the
		 * argument of the operation.
		 */

		mudstate.lock_nest_lev++;
		if (mudstate.lock_nest_lev >= mudconf.lock_nest_lim) {
			STARTLOG(LOG_BUGS, "BUG", "LOCK")
				log_name_and_loc(player);
			log_printf(": Lock exceeded recursion limit.");
			ENDLOG
				notify(player, "Sorry, broken lock!");
			mudstate.lock_nest_lev--;
			return (0);
		}
		if ((b->sub1->type != BOOLEXP_CONST) || (b->sub1->thing < 0)) {
			STARTLOG(LOG_BUGS, "BUG", "LOCK")
				log_name_and_loc(player);
			log_printf(": Lock had bad indirection (%c, type %d)",
				   INDIR_TOKEN, b->sub1->type);
			ENDLOG
				notify(player, "Sorry, broken lock!");
			mudstate.lock_nest_lev--;
			return (0);
		}
		key = atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags, &alen);
		lock_originator = thing;
		c = eval_boolexp_atr(player, b->sub1->thing, from, key);
		lock_originator = NOTHING;
		free_lbuf(key);
		mudstate.lock_nest_lev--;
		return (c);
	case BOOLEXP_CONST:
		return (b->thing == player ||
			member(b->thing, Contents(player)));
	case BOOLEXP_ATR:
		a = atr_num(b->thing);
		if (!a)
			return 0;	/*
					 * no such attribute 
					 */

		/*
		 * First check the object itself, then its contents 
		 */

		if (check_attr(player, from, a, (char *)b->sub1))
			return 1;
		DOLIST(obj, Contents(player)) {
			if (check_attr(obj, from, a, (char *)b->sub1))
				return 1;
		}
		return 0;
	case BOOLEXP_EVAL:
		a = atr_num(b->thing);
		if (!a)
			return 0;	/*
					 * no such attribute 
					 */
		source = from;
		buff = atr_pget(from, a->number, &aowner, &aflags, &alen);
		if (!*buff) {
			free_lbuf(buff);
			buff = atr_pget(thing, a->number, &aowner,
					&aflags, &alen);
			source = thing;
		}
		checkit = 0;
		
		if ((a->number == A_NAME) || (a->number == A_LCONTROL)) {
			checkit = 1;
		} else if (Read_attr(source, source, a, aowner, aflags)) {
			checkit = 1;
		}
		if (checkit) {
		        preserve = save_global_regs("eval_boolexp_save");
			buff2 = bp = alloc_lbuf("eval_boolexp");
			str = buff;
			exec(buff2, &bp, source, 
			     ((lock_originator == NOTHING) ?
			      player : lock_originator),
			     player, EV_FCHECK | EV_EVAL | EV_TOP,
			     &str, (char **)NULL, 0);
 			restore_global_regs("eval_boolexp_save", preserve);
			checkit = !string_compare(buff2, (char *)b->sub1);
			free_lbuf(buff2);
		}
		free_lbuf(buff);
		return checkit;
	case BOOLEXP_IS:

		/*
		 * If an object check, do that 
		 */

		if (b->sub1->type == BOOLEXP_CONST)
			return (b->sub1->thing == player);

		/*
		 * Nope, do an attribute check 
		 */

		a = atr_num(b->sub1->thing);
		if (!a)
			return 0;
		return (check_attr(player, from, a, (char *)(b->sub1)->sub1));
	case BOOLEXP_CARRY:

		/*
		 * If an object check, do that 
		 */

		if (b->sub1->type == BOOLEXP_CONST)
			return (member(b->sub1->thing, Contents(player)));

		/*
		 * Nope, do an attribute check 
		 */

		a = atr_num(b->sub1->thing);
		if (!a)
			return 0;
		DOLIST(obj, Contents(player)) {
			if (check_attr(obj, from, a, (char *)(b->sub1)->sub1))
				return 1;
		}
		return 0;
	case BOOLEXP_OWNER:
		return (Owner(b->sub1->thing) == Owner(player));
	default:
	    fprintf(mainlog_fp, "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n");
	    abort();		/* bad type */
	    return 0;		/* NOTREACHED */
	}
}

int eval_boolexp_atr(player, thing, from, key)
dbref player, thing, from;
char *key;
{
	BOOLEXP *b;
	int ret_value;

	b = parse_boolexp(player, key, 1);
	if (b == NULL) {
		ret_value = 1;
	} else {
		ret_value = eval_boolexp(player, thing, from, b);
		free_boolexp(b);
	}
	return (ret_value);
}

/* If the parser returns TRUE_BOOLEXP, you lose TRUE_BOOLEXP cannot be typed
 * in by the user; use @unlock instead
 */

static char *parsebuf, *parsestore;
static dbref parse_player;

static void NDECL(skip_whitespace)
{
	while (*parsebuf && isspace(*parsebuf))
		parsebuf++;
}

static BOOLEXP *NDECL(parse_boolexp_E);		/* defined below */

static BOOLEXP *test_atr(s)
char *s;
{
	ATTR *attrib;
	BOOLEXP *b;
	char *buff, *s1;
	int anum, locktype;

	buff = alloc_lbuf("test_atr");
	StringCopy(buff, s);
	for (s = buff; *s && (*s != ':') && (*s != '/'); s++) ;
	if (!*s) {
		free_lbuf(buff);
		return ((BOOLEXP *) NULL);
	}
	if (*s == '/')
		locktype = BOOLEXP_EVAL;
	else
		locktype = BOOLEXP_ATR;

	*s++ = '\0';
	/*
	 * see if left side is valid attribute.  Access to attr is checked on 
	 * eval.  Also allow numeric references to attributes.  It can't hurt
	 * us, and lets us import stuff that stores attr locks by number
	 * instead of by name. 
	 */
	if (!(attrib = atr_str(buff))) {

		/*
		 * Only #1 can lock on numbers 
		 */
		if (!God(parse_player)) {
			free_lbuf(buff);
			return ((BOOLEXP *) NULL);
		}
		s1 = buff;
		for (s1 = buff; isdigit(*s1); s1++) ;
		if (*s1) {
			free_lbuf(buff);
			return ((BOOLEXP *) NULL);
		}
		anum = atoi(buff);
		if (anum <= 0) {
			free_lbuf(buff);
			return ((BOOLEXP *) NULL);
		}
	} else {
		anum = attrib->number;
	}

	/*
	 * made it now make the parse tree node 
	 */
	b = alloc_bool("test_atr");
	b->type = locktype;
	b->thing = (dbref) anum;
	b->sub1 = (BOOLEXP *) XSTRDUP(s, "test_atr.sub1");
	free_lbuf(buff);
	return (b);
}

/*
 * L -> (E); L -> object identifier 
 */

static BOOLEXP *NDECL(parse_boolexp_L)
{
	BOOLEXP *b;
	char *p, *buf;
	MSTATE mstate;


	buf = NULL;
	skip_whitespace();

	switch (*parsebuf) {
	case '(':
		parsebuf++;
		b = parse_boolexp_E();
		skip_whitespace();
		if (b == TRUE_BOOLEXP || *parsebuf++ != ')') {
			free_boolexp(b);
			return TRUE_BOOLEXP;
		}
		break;
	default:

		/* must have hit an object ref.  Load the name into our 
		 * buffer */

		buf = alloc_lbuf("parse_boolexp_L");
		p = buf;
		while (*parsebuf && (*parsebuf != AND_TOKEN) &&
		       (*parsebuf != OR_TOKEN) && (*parsebuf != ')')) {
			*p++ = *parsebuf++;
		}

		/* strip trailing whitespace */

		*p-- = '\0';
		while (isspace(*p))
			*p-- = '\0';

		/* check for an attribute */

		if ((b = test_atr(buf)) != NULL) {
			free_lbuf(buf);
			return (b);
		}
		b = alloc_bool("parse_boolexp_L");
		b->type = BOOLEXP_CONST;

		/* Do the match.
		 * If we are parsing a boolexp that was a stored lock then
		 * we know that object refs are all dbrefs, so we skip the
		 * expensive match code.
		 */


		if (!mudstate.standalone) {
			if (parsing_internal) {
				if (buf[0] != '#') {
					free_lbuf(buf);
					free_bool(b);
					return TRUE_BOOLEXP;
				}
				b->thing = atoi(&buf[1]);
				if (!Good_obj(b->thing)) {
					free_lbuf(buf);
					free_bool(b);
					return TRUE_BOOLEXP;
				}
			} else {
				save_match_state(&mstate);
				init_match(parse_player, buf, TYPE_THING);
				match_everything(MAT_EXIT_PARENTS);
				b->thing = match_result();
				restore_match_state(&mstate);
			}

			if (b->thing == NOTHING) {
				notify(parse_player,
				       tprintf("I don't see %s here.", buf));
				free_lbuf(buf);
				free_bool(b);
				return TRUE_BOOLEXP;
			}
			if (b->thing == AMBIGUOUS) {
				notify(parse_player,
				       tprintf("I don't know which %s you mean!",
					       buf));
				free_lbuf(buf);
				free_bool(b);
				return TRUE_BOOLEXP;
			}
		} else {
			if (buf[0] != '#') {
				free_lbuf(buf);
				free_bool(b);
				return TRUE_BOOLEXP;
			}
			b->thing = atoi(&buf[1]);
			if (b->thing < 0) {
				free_lbuf(buf);
				free_bool(b);
				return TRUE_BOOLEXP;
			}
		}
		free_lbuf(buf);
	}
	return b;
}

/*
 * F -> !F; F -> @L; F -> =L; F -> +L; F -> $L 
 */
/*
 * The argument L must be type BOOLEXP_CONST 
 */

static BOOLEXP *NDECL(parse_boolexp_F)
{
	BOOLEXP *b2;

	skip_whitespace();
	switch (*parsebuf) {
	case NOT_TOKEN:
		parsebuf++;
		b2 = alloc_bool("parse_boolexp_F.not");
		b2->type = BOOLEXP_NOT;
		if ((b2->sub1 = parse_boolexp_F()) == TRUE_BOOLEXP) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else
			return (b2);
	case INDIR_TOKEN:
		parsebuf++;
		b2 = alloc_bool("parse_boolexp_F.indir");
		b2->type = BOOLEXP_INDIR;
		b2->sub1 = parse_boolexp_L();
		if ((b2->sub1) == TRUE_BOOLEXP) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else if ((b2->sub1->type) != BOOLEXP_CONST) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else
			return (b2);
	case IS_TOKEN:
		parsebuf++;
		b2 = alloc_bool("parse_boolexp_F.is");
		b2->type = BOOLEXP_IS;
		b2->sub1 = parse_boolexp_L();
		if ((b2->sub1) == TRUE_BOOLEXP) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else if (((b2->sub1->type) != BOOLEXP_CONST) &&
			   ((b2->sub1->type) != BOOLEXP_ATR)) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else
			return (b2);
	case CARRY_TOKEN:
		parsebuf++;
		b2 = alloc_bool("parse_boolexp_F.carry");
		b2->type = BOOLEXP_CARRY;
		b2->sub1 = parse_boolexp_L();
		if ((b2->sub1) == TRUE_BOOLEXP) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else if (((b2->sub1->type) != BOOLEXP_CONST) &&
			   ((b2->sub1->type) != BOOLEXP_ATR)) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else
			return (b2);
	case OWNER_TOKEN:
		parsebuf++;
		b2 = alloc_bool("parse_boolexp_F.owner");
		b2->type = BOOLEXP_OWNER;
		b2->sub1 = parse_boolexp_L();
		if ((b2->sub1) == TRUE_BOOLEXP) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else if ((b2->sub1->type) != BOOLEXP_CONST) {
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		} else
			return (b2);
	default:
		return (parse_boolexp_L());
	}
}

/*
 * T -> F; T -> F & T 
 */

static BOOLEXP *NDECL(parse_boolexp_T)
{
	BOOLEXP *b, *b2;

	if ((b = parse_boolexp_F()) != TRUE_BOOLEXP) {
		skip_whitespace();
		if (*parsebuf == AND_TOKEN) {
			parsebuf++;

			b2 = alloc_bool("parse_boolexp_T");
			b2->type = BOOLEXP_AND;
			b2->sub1 = b;
			if ((b2->sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
				free_boolexp(b2);
				return TRUE_BOOLEXP;
			}
			b = b2;
		}
	}
	return b;
}

/*
 * E -> T; E -> T | E 
 */

static BOOLEXP *NDECL(parse_boolexp_E)
{
	BOOLEXP *b, *b2;

	if ((b = parse_boolexp_T()) != TRUE_BOOLEXP) {
		skip_whitespace();
		if (*parsebuf == OR_TOKEN) {
			parsebuf++;

			b2 = alloc_bool("parse_boolexp_E");
			b2->type = BOOLEXP_OR;
			b2->sub1 = b;
			if ((b2->sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
				free_boolexp(b2);
				return TRUE_BOOLEXP;
			}
			b = b2;
		}
	}
	return b;
}

BOOLEXP *parse_boolexp(player, buf, internal)
dbref player;
const char *buf;
int internal;
{
	char *p;
	int num_opens = 0;
	BOOLEXP *ret;
	
	if (!internal) {
	    /* Don't allow funky characters in locks.
	     * Don't allow unbalanced parentheses.
	     */
	    for (p = (char *) buf; *p; p++) {
		if ((*p == '\t') || (*p == '\r') || (*p == '\n') ||
		    (*p == ESC_CHAR)) {
		    return (TRUE_BOOLEXP);
		}
		if (*p == '(') {
		    num_opens++;
		} else if (*p == ')') {
		    if (num_opens > 0) {
			num_opens--;
		    } else {
			return (TRUE_BOOLEXP);
		    }
		}
	    }
	    if (num_opens != 0)
		return (TRUE_BOOLEXP);
	}

	if ((buf == NULL) || (*buf == '\0'))
		return (TRUE_BOOLEXP);

	parsestore = parsebuf = alloc_lbuf("parse_boolexp");
	StringCopy(parsebuf, buf);
	parse_player = player;
	if (!mudstate.standalone)
		parsing_internal = internal;
	ret = parse_boolexp_E();
	free_lbuf(parsestore);
	return ret;
}
