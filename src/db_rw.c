/* db_rw.c - flatfile implementation */
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

#include "vattr.h"	/* required by code */
#include "attrs.h"	/* required by code */

#ifdef STANDALONE
#include "powers.h"	/* required by code */
#endif

extern void FDECL(db_grow, (dbref));

extern struct object *db;

static int g_version;
static int g_format;
static int g_flags;

/* ---------------------------------------------------------------------------
 * getboolexp1: Get boolean subexpression from file.
 */

BOOLEXP *getboolexp1(f)
FILE *f;
{
	BOOLEXP *b;
	char *buff, *s;
	int c, d, anum;

	c = getc(f);
	switch (c) {
	case '\n':
		ungetc(c, f);
		return TRUE_BOOLEXP;
		/* break; */
	case EOF:
	        fprintf(mainlog_fp, "ABORT! db_rw.c, unexpected EOF in boolexp in getboolexp1().\n");
		abort();
		break;
	case '(':
		b = alloc_bool("getboolexp1.openparen");
		switch (c = getc(f)) {
		case NOT_TOKEN:
			b->type = BOOLEXP_NOT;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		case INDIR_TOKEN:
			b->type = BOOLEXP_INDIR;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		case IS_TOKEN:
			b->type = BOOLEXP_IS;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		case CARRY_TOKEN:
			b->type = BOOLEXP_CARRY;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		case OWNER_TOKEN:
			b->type = BOOLEXP_OWNER;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		default:
			ungetc(c, f);
			b->sub1 = getboolexp1(f);
			if ((c = getc(f)) == '\n')
				c = getc(f);
			switch (c) {
			case AND_TOKEN:
				b->type = BOOLEXP_AND;
				break;
			case OR_TOKEN:
				b->type = BOOLEXP_OR;
				break;
			default:
				goto error;
			}
			b->sub2 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
				d = getc(f);
			if (d != ')')
				goto error;
			return b;
		}
	case '-':		/* obsolete NOTHING key, eat it */
	        while ((c = getc(f)) != '\n') {
		    if (c == EOF) {
			fprintf(mainlog_fp, "ABORT! db_rw.c, unexpected EOF in getboolexp1().\n");
			abort();
		    }
		}
		ungetc(c, f);
		return TRUE_BOOLEXP;
	case '"':
		ungetc(c, f);
		buff = alloc_lbuf("getboolexp_quoted");
		StringCopy(buff, getstring_noalloc(f, 1));
		c = fgetc(f);
		if (c == EOF) {
			free_lbuf(buff);
			return TRUE_BOOLEXP;
		}

		b = alloc_bool("getboolexp1_quoted");
		anum = mkattr(buff);
		if (anum <= 0) {
			free_bool(b);
			free_lbuf(buff);
			goto error;
		}
		free_lbuf(buff);
		b->thing = anum;

		/* if last character is : then this is an attribute lock. A 
		 * last character of / means an eval lock 
		 */

		if ((c == ':') || (c == '/')) {
			if (c == '/')
				b->type = BOOLEXP_EVAL;
			else
				b->type = BOOLEXP_ATR;
			b->sub1 = (BOOLEXP *) XSTRDUP(getstring_noalloc(f, 1), "getboolexp1.attr_lock");
		}
		return b;
	default:		/* dbref or attribute */
		ungetc(c, f);
		b = alloc_bool("getboolexp1.default");
		b->type = BOOLEXP_CONST;
		b->thing = 0;

		/* This is either an attribute, eval, or constant lock.
		 * Constant locks are of the form <num>, while attribute and 
		 * eval locks are of the form <anam-or-anum>:<string> or
		 * <aname-or-anum>/<string> respectively. The
		 * characters <nl>, |, and & terminate the string. 
		 */

		if (isdigit(c)) {
			while (isdigit(c = getc(f))) {
				b->thing = b->thing * 10 + c - '0';
			}
		} else if (isalpha(c)) {
			buff = alloc_lbuf("getboolexp1.atr_name");
			for (s = buff;
			     ((c = getc(f)) != EOF) && (c != '\n') &&
			     (c != ':') && (c != '/');
			     *s++ = c) ;
			if (c == EOF) {
				free_lbuf(buff);
				free_bool(b);
				goto error;
			}
			*s = '\0';

			/* Look the name up as an attribute.  If not found,
			 * create a new attribute. 
			 */

			anum = mkattr(buff);
			if (anum <= 0) {
				free_bool(b);
				free_lbuf(buff);
				goto error;
			}
			free_lbuf(buff);
			b->thing = anum;
		} else {
			free_bool(b);
			goto error;
		}

		/* if last character is : then this is an attribute lock. A 
		 * last character of / means an eval lock 
		 */

		if ((c == ':') || (c == '/')) {
			if (c == '/')
				b->type = BOOLEXP_EVAL;
			else
				b->type = BOOLEXP_ATR;
			buff = alloc_lbuf("getboolexp1.attr_lock");
			for (s = buff;
			     ((c = getc(f)) != EOF) && (c != '\n') && (c != ')') &&
			     (c != OR_TOKEN) && (c != AND_TOKEN);
			     *s++ = c) ;
			if (c == EOF)
				goto error;
			*s++ = 0;
			b->sub1 = (BOOLEXP *) XSTRDUP(buff, "getboolexp1.attr_lock");
			free_lbuf(buff);
		}
		ungetc(c, f);
		return b;
	}

      error:
	fprintf(mainlog_fp,
		"ABORT! db_rw.c, reached error case in getboolexp1().\n");
	abort();		/* bomb out */
	return TRUE_BOOLEXP;	/* NOTREACHED */
}

/* ---------------------------------------------------------------------------
 * getboolexp: Read a boolean expression from the flat file.
 */

static BOOLEXP *getboolexp(f)
FILE *f;
{
	BOOLEXP *b;
	char c;

	b = getboolexp1(f);
	if (getc(f) != '\n') {
	    fprintf(mainlog_fp, "ABORT! db_rw.c, parse error in getboolexp().\n");
	    abort();	/* parse error, we lose */
	}

	if ((c = getc(f)) != '\n')
		ungetc(c, f);

	return b;
}

#ifdef STANDALONE
/* ---------------------------------------------------------------------------
 * unscramble_attrnum: Fix up attribute numbers from foreign muds
 */

static int unscramble_attrnum(attrnum)
int attrnum;
{
	char anam[4];

	switch (g_format) {
	case F_MUSH:
		/* TinyMUSH 2.2:  Deal with different attribute numbers. */

		switch (attrnum) {
		    case 208:
			return A_NEWOBJS;
			break;
		    case 209:
			return A_LCON_FMT;
			break;
		    case 210:
			return A_LEXITS_FMT;
			break;
		    case 211:
			return A_PROGCMD;
			break;
		    default:
			return attrnum;
		}
	default:
		return attrnum;
	}
}
#endif

/* ---------------------------------------------------------------------------
 * get_list: Read attribute list from flat file.
 */

static int get_list(f, i, new_strings)
FILE *f;
dbref i;
int new_strings;
{
	dbref atr;
	int c;
	char *buff;
#ifdef STANDALONE
	int aflags, xflags, anum;
	char *buf2, *buf2p, *ownp, *flagp;
	dbref aowner;
#endif

	buff = alloc_lbuf("get_list");
	while (1) {
		switch (c = getc(f)) {
		case '>':	/* read # then string */
#ifdef STANDALONE
			atr = unscramble_attrnum(getref(f));
#else
			atr = getref(f);
#endif
			if (atr > 0) {
				/* Store the attr */

				atr_add_raw(i, atr,
					 (char *)getstring_noalloc(f, new_strings));
			} else {
				/* Silently discard */

				getstring_noalloc(f, new_strings);
			}
			break;
		case '\n':	/* ignore newlines. They're due to v(r). */
			break;
		case '<':	/* end of list */
			free_lbuf(buff);
			c = getc(f);
			if (c != '\n') {
				ungetc(c, f);
				fprintf(mainlog_fp,
					"No line feed on object %d\n", i);
				return 1;
			}
			return 1;
		default:
			fprintf(mainlog_fp,
				"Bad character '%c' when getting attributes on object %d\n",
				c, i);
			/* We've found a bad spot.  I hope things aren't
			 * too bad. 
			 */

			(void)getstring_noalloc(f, new_strings);
		}
	}
	return 1;		/* NOTREACHED */
}

/* ---------------------------------------------------------------------------
 * putbool_subexp: Write a boolean sub-expression to the flat file.
 */
static void putbool_subexp(f, b)
FILE *f;
BOOLEXP *b;
{
	ATTR *va;

	switch (b->type) {
	case BOOLEXP_IS:
		putc('(', f);
		putc(IS_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_CARRY:
		putc('(', f);
		putc(CARRY_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_INDIR:
		putc('(', f);
		putc(INDIR_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_OWNER:
		putc('(', f);
		putc(OWNER_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_AND:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(AND_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_OR:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(OR_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_NOT:
		putc('(', f);
		putc(NOT_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_CONST:
		fprintf(f, "%d", b->thing);
		break;
	case BOOLEXP_ATR:
		va = atr_num(b->thing);
		if (va) {
			fprintf(f, "%s:%s", va->name, (char *)b->sub1);
		} else {
			fprintf(f, "%d:%s\n", b->thing, (char *)b->sub1);
		}
		break;
	case BOOLEXP_EVAL:
		va = atr_num(b->thing);
		if (va) {
			fprintf(f, "%s/%s\n", va->name, (char *)b->sub1);
		} else {
			fprintf(f, "%d/%s\n", b->thing, (char *)b->sub1);
		}
		break;
	default:
		fprintf(mainlog_fp, "Unknown boolean type in putbool_subexp: %d\n",
			b->type);
	}
}

/* ---------------------------------------------------------------------------
 * putboolexp: Write boolean expression to the flat file.
 */

void putboolexp(f, b)
FILE *f;
BOOLEXP *b;
{
	if (b != TRUE_BOOLEXP) {
		putbool_subexp(f, b);
	}
	putc('\n', f);
}

#ifdef STANDALONE
/* ---------------------------------------------------------------------------
 * upgrade_flags: Convert foreign flags to MUSH format.
 */

static void upgrade_flags(flags1, flags2, flags3, thing, db_format, db_version)
FLAG *flags1, *flags2, *flags3;
dbref thing;
int db_format, db_version;
{
	FLAG f1, f2, f3, newf1, newf2, newf3;

	f1 = *flags1;
	f2 = *flags2;
	f3 = *flags3;
	newf1 = 0;
	newf2 = 0;
	newf3 = 0;
	if ((db_format == F_MUSH) && (db_version >= 3)) {
		newf1 = f1;
		newf2 = f2;
		newf3 = 0;

		/* Then we have to do the 2.2 to 3.0 flag conversion */

		if (newf1 & ROYALTY) {
		    newf1 &= ~ROYALTY;
		    newf2 |= CONTROL_OK;
		}
		if (newf2 & HAS_COMMANDS) {
		    newf2 &= ~HAS_COMMANDS;
		    newf2 |= NOBLEED;
		}
		if (newf2 & AUDITORIUM) {
		    newf2 &= ~AUDITORIUM;
		    newf2 |= ZONE_PARENT;
		}
		if (newf2 & ANSI) {
		    newf2 &= ~ANSI;
		    newf2 |= STOP_MATCH;
		}
		if (newf2 & HEAD_FLAG) {
		    newf2 &= ~HEAD_FLAG;
		    newf2 |= HAS_COMMANDS;
		}
		if (newf2 & FIXED) {
		    newf2 &= ~FIXED;
		    newf2 |= BOUNCE;
		}
		if (newf2 & STAFF) {
		    newf2 &= STAFF;
		    newf2 |= HTML;
		}
		if (newf2 & HAS_DAILY) {
		    newf2 &= ~HAS_DAILY;
		    /* This is the unimplemented TICKLER flag. */
		}
		if (newf2 & GAGGED) {
		    newf2 &= ~GAGGED;
		    newf2 |= ANSI;
		}
		if (newf2 & WATCHER) {
		    newf2 &= ~WATCHER;
		    s_Powers(thing, Powers(thing) | POW_BUILDER);
		}
	} else if (db_format == F_MUX) {

	    /* TinyMUX to 3.0 flag conversion */

	    newf1 = f1;
	    newf2 = f2;
	    newf3 = f3;

	    if (newf2 & ZONE_PARENT) {
		/* This used to be an object set NO_COMMAND. We unset the
		 * flag.
		 */
		newf2 &= ~ZONE_PARENT;
	    } else {
		/* And if it wasn't NO_COMMAND, then it should be COMMANDS. */
		newf2 |= HAS_COMMANDS;
	    }

	    if (newf2 & WATCHER) {
		/* This used to be the COMPRESS flag, which didn't do
		 * anything.
		 */
		newf2 &= ~WATCHER;
	    }

	    if ((newf1 & MONITOR) && ((newf1 & TYPE_MASK) == TYPE_PLAYER)) {
		/* Players set MONITOR should be set WATCHER as well. */
		newf2 |= WATCHER;
	    }

	} else if (db_format == F_TINYMUSH) {
		/* Native TinyMUSH 3.0 database.
		 * The only thing we have to do is clear the redirection
		 * flag, as nothing is ever redirected at startup.
		 */
		newf1 = f1;
		newf2 = f2;
		newf3 = f3 & ~HAS_REDIRECT;
	}

	newf2 = newf2 & ~FLOATING; /* this flag is now obsolete */

	*flags1 = newf1;
	*flags2 = newf2;
	*flags3 = newf3;
	return;
}
#endif

/* ---------------------------------------------------------------------------
 * efo_convert: Fix things up for Exits-From-Objects
 */

void NDECL(efo_convert)
{
	int i;
	dbref link;

	DO_WHOLE_DB(i) {
		switch (Typeof(i)) {
		case TYPE_PLAYER:
		case TYPE_THING:

			/* swap Exits and Link */

			link = Link(i);
			s_Link(i, Exits(i));
			s_Exits(i, link);
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * fix_mux_zones: Convert MUX-style zones to 3.0-style zones.
 */

#ifdef STANDALONE
static void fix_mux_zones()
{
    /* For all objects in the database where Zone(thing) != NOTHING,
     * set the CONTROL_OK flag on them.
     *
     * For all objects in the database that are ZMOs (that have other
     * objects zoned to them), copy the EnterLock of those objects to
     * the ControlLock.
     */

    int i;
    int *zmarks;
    char *astr;

    zmarks = (int *) XCALLOC(mudstate.db_top, sizeof(int), "fix_mux_zones");

    DO_WHOLE_DB(i) {
	if (Zone(i) != NOTHING) {
	    s_Flags2(i, Flags2(i) | CONTROL_OK);
	    zmarks[Zone(i)] = 1;
	}
    }

    DO_WHOLE_DB(i) {
	if (zmarks[i]) {
	    astr = atr_get_raw(i, A_LENTER);
	    if (astr) {
		atr_add_raw(i, A_LCONTROL, astr);
	    }
	}
    }

    XFREE(zmarks, "fix_mux_zones");
}
#endif /* STANDALONE */

/* ---------------------------------------------------------------------------
 * fix_typed_quotas: Explode standard quotas into typed quotas
 */

#ifdef STANDALONE
static void fix_typed_quotas()
{
	/* If we have a pre-2.2 or MUX database, only the QUOTA and RQUOTA
	 * attributes  exist. For simplicity's sake, we assume that players
	 * will have the  same quotas for all types, equal to the current
	 * value. This is  going to produce incorrect values for RQUOTA;
	 * this is easily fixed  by a @quota/fix done from within-game. 
	 *
	 * If we have a early beta 2.2 release, we have quotas which are
	 * spread out over ten attributes. We're going to have to grab
	 * those, make the new quotas, and then delete the old attributes.
	 */

	int i;
	char *qbuf, *rqbuf;

	DO_WHOLE_DB(i) {
		if (isPlayer(i)) {
			qbuf = atr_get_raw(i, A_QUOTA);
			rqbuf = atr_get_raw(i, A_RQUOTA);
			if (!qbuf || !*qbuf)
				qbuf = (char *)"1";
			if (!rqbuf || !*rqbuf)
				rqbuf = (char *)"0";
			atr_add_raw(i, A_QUOTA,
				    tprintf("%s %s %s %s %s",
					    qbuf, qbuf, qbuf, qbuf, qbuf));
			atr_add_raw(i, A_RQUOTA,
				    tprintf("%s %s %s %s %s",
					rqbuf, rqbuf, rqbuf, rqbuf, rqbuf));
		}
	}
}

dbref db_convert(f, db_format, db_version, db_flags)
FILE *f;
int *db_format, *db_version, *db_flags;
{
	dbref i, anum;
	char ch;
	const char *tstr;
	int header_gotten, size_gotten, nextattr_gotten;
	int read_attribs, read_name, read_zone, read_link, read_key, read_parent;
	int read_extflags, read_3flags, read_money, read_timestamps, read_new_strings;
	char peek;
	int read_powers, read_powers_player, read_powers_any;
	int has_typed_quotas, has_visual_attrs;
	int deduce_version, deduce_name, deduce_zone, deduce_timestamps;
	int aflags, f1, f2, f3;
	BOOLEXP *tempbool;
	time_t tmptime;
#ifndef NO_TIMECHECKING
	struct timeval obj_time;
#endif

	header_gotten = 0;
	size_gotten = 0;
	nextattr_gotten = 0;
	g_format = F_UNKNOWN;
	g_version = 0;
	g_flags = 0;
	read_attribs = 1;
	read_name = 1;
	read_zone = 0;
	read_link = 0;
	read_key = 1;
	read_parent = 0;
	read_money = 1;
	read_extflags = 0;
	read_3flags = 0;
	has_typed_quotas = 0;
	has_visual_attrs = 0;
	read_timestamps = 0;
	read_new_strings = 0;
	read_powers = 0;
	read_powers_player = 0;
	read_powers_any = 0;
	deduce_version = 1;
	deduce_zone = 1;
	deduce_name = 1;
	deduce_timestamps = 1;
	fprintf(mainlog_fp, "Reading ");
	db_free();
	for (i = 0;; i++) {

		if (!(i % 100)) {
			fputc('.', mainlog_fp);
		}

		switch (ch = getc(f)) {
		case '-':	/* Misc tag */
			switch (ch = getc(f)) {	
			case 'R':	/* Record number of players */
				mudstate.record_players = getref(f);
				break;
			default:
				(void)getstring_noalloc(f, 0);
			}
			break;
		case '+':	/* MUX and MUSH header */

		    ch = getc(f); /* 2nd char selects type */

		    if ((ch == 'V') || (ch == 'X') || (ch == 'T')) {

			/* The following things are common across 2.x, MUX,
			 * and 3.0.
			 */
		        
			 if (header_gotten) {
			     fprintf(mainlog_fp,
				     "\nDuplicate MUSH version header entry at object %d, ignored.\n",
				     i);
			     tstr = getstring_noalloc(f, 0);
			     break;
			 }
			 header_gotten = 1;
			 deduce_version = 0;
			 g_version = getref(f);
			 
			 /* Otherwise extract feature flags */

			 if (g_version & V_GDBM) {
			     read_attribs = 0;
			     read_name = !(g_version & V_ATRNAME);
			 }
			 read_zone = (g_version & V_ZONE);
			 read_link = (g_version & V_LINK);
			 read_key = !(g_version & V_ATRKEY);
			 read_parent = (g_version & V_PARENT);
			 read_money = !(g_version & V_ATRMONEY);
			 read_extflags = (g_version & V_XFLAGS);
			 has_typed_quotas = (g_version & V_TQUOTAS);
			 read_timestamps = (g_version & V_TIMESTAMPS);
			 has_visual_attrs = (g_version & V_VISUALATTRS);
			 g_flags = g_version & ~V_MASK;

			 deduce_name = 0;
			 deduce_version = 0;
			 deduce_zone = 0;
		    }

		    /* More generic switch. */

		    switch (ch) {
			case 'T':	/* 3.0 VERSION */
			    g_format = F_TINYMUSH;
			    read_3flags = (g_version & V_3FLAGS);
			    read_powers = (g_version & V_POWERS);
			    read_new_strings = (g_version & V_QUOTED);
			    g_version &= V_MASK;
			    break;

			case 'V':	/* 2.0 VERSION */
			    g_format = F_MUSH;
			    g_version &= V_MASK;
			    break;

			case 'X':	/* MUX VERSION */
			    g_format = F_MUX;
			    read_3flags = (g_version & V_3FLAGS);
			    read_powers = (g_version & V_POWERS);
			    read_new_strings = (g_version & V_QUOTED);
			    g_version &= V_MASK;
			    break;

			case 'S':	/* SIZE */
				if (size_gotten) {
					fprintf(mainlog_fp,
						"\nDuplicate size entry at object %d, ignored.\n",
						i);
					tstr = getstring_noalloc(f, 0);
				} else {
					mudstate.min_size = getref(f);
				}
				size_gotten = 1;
				break;
			case 'A':	/* USER-NAMED ATTRIBUTE */
				anum = getref(f);
				tstr = getstring_noalloc(f, read_new_strings);
				if (isdigit(*tstr)) {
					aflags = 0;
					while (isdigit(*tstr))
						aflags = (aflags * 10) +
							(*tstr++ - '0');
					tstr++;		/* skip ':' */
					if (!has_visual_attrs) {
					    /* If not AF_ODARK, is AF_VISUAL.
					     * Strip AF_ODARK.
					     */
					    if (aflags & AF_ODARK)
						aflags &= ~AF_ODARK;
					    else
						aflags |= AF_VISUAL;
					}
				} else {
					aflags = mudconf.vattr_flags;
				}
				vattr_define((char *)tstr, anum, aflags);
				break;
			case 'F':	/* OPEN USER ATTRIBUTE SLOT */
				anum = getref(f);
				break;
			case 'N':	/* NEXT ATTR TO ALLOC WHEN NO
					 * FREELIST 
					 */
				if (nextattr_gotten) {
					fprintf(mainlog_fp,
						"\nDuplicate next free vattr entry at object %d, ignored.\n",
						i);
					tstr = getstring_noalloc(f, 0);
				} else {
					mudstate.attr_next = getref(f);
					nextattr_gotten = 1;
				}
				break;
			default:
				fprintf(mainlog_fp,
					"\nUnexpected character '%c' in MUSH header near object #%d, ignored.\n",
					ch, i);
				tstr = getstring_noalloc(f, 0);
			}
			break;
		case '!':	/* MUX entry/MUSH entry */
			if (deduce_version) {
				g_format = F_TINYMUSH;
				g_version = 1;
				deduce_name = 0;
				deduce_zone = 0;
				deduce_version = 0;
			} else if (deduce_zone) {
				deduce_zone = 0;
				read_zone = 0;
			}
			i = getref(f);
			db_grow(i + 1);

#ifndef NO_TIMECHECKING
			obj_time.tv_sec = obj_time.tv_usec = 0;
			s_Time_Used(i, obj_time);
#endif
			s_StackCount(i, 0);
			s_VarsCount(i, 0);
			s_StructCount(i, 0);
			s_InstanceCount(i, 0);

			if (read_name) {
				tstr = getstring_noalloc(f, read_new_strings);
				if (deduce_name) {
					if (isdigit(*tstr)) {
						read_name = 0;
						s_Location(i, atoi(tstr));
					} else {
						s_Name(i, (char *)tstr);
						s_Location(i, getref(f));
					}
					deduce_name = 0;
				} else {
					s_Name(i, (char *)tstr);
					s_Location(i, getref(f));
				}
			} else {
				s_Location(i, getref(f));
			}

			if (read_zone)
				s_Zone(i, getref(f));
/* else s_Zone(i, NOTHING); */

			/* CONTENTS and EXITS */

			s_Contents(i, getref(f));
			s_Exits(i, getref(f));

			/* LINK */

			if (read_link)
				s_Link(i, getref(f));
			else
				s_Link(i, NOTHING);

			/* NEXT */

			s_Next(i, getref(f));

			/* LOCK */

			if (read_key) {
				tempbool = getboolexp(f);
				atr_add_raw(i, A_LOCK,
				unparse_boolexp_quiet(1, tempbool));
				free_boolexp(tempbool);
			}
			/* OWNER */

			s_Owner(i, getref(f));

			/* PARENT */

			if (read_parent) {
				s_Parent(i, getref(f));
			} else {
				s_Parent(i, NOTHING);
			}

			/* PENNIES */

			if (read_money)		
				s_Pennies(i, getref(f));

			/* FLAGS */

			f1 = getref(f);
			if (read_extflags)
				f2 = getref(f);
			else
				f2 = 0;
				
			if (read_3flags)
				f3 = getref(f);
			else
				f3 = 0;

			upgrade_flags(&f1, &f2, &f3, i, g_format, g_version);
			s_Flags(i, f1);
			s_Flags2(i, f2);
			s_Flags3(i, f3);

			if (read_powers) {
				f1 = getref(f);
				f2 = getref(f);
				s_Powers(i, f1);
				s_Powers2(i, f2);
			}

			if (read_timestamps) {
			    tmptime = (time_t) getlong(f);
			    s_AccessTime(i, tmptime);
			    tmptime = (time_t) getlong(f);
			    s_ModTime(i, tmptime);
			} else {
			    AccessTime(i) = ModTime(i) = time(NULL);
			}
				
			/* ATTRIBUTES */

			if (read_attribs) {
				if (!get_list(f, i, read_new_strings)) {
					fprintf(mainlog_fp,
						"\nError reading attrs for object #%d\n",
						i);
					return -1;
				}
			}

			/* check to see if it's a player */

			if (Typeof(i) == TYPE_PLAYER) {
				c_Connected(i);
			}
			break;
		case '*':	/* EOF marker */
			tstr = getstring_noalloc(f, 0);
			if (strcmp(tstr, "**END OF DUMP***")) {
				fprintf(mainlog_fp,
					"\nBad EOF marker at object #%d\n",
					i);
				return -1;
			} else {
				fprintf(mainlog_fp, "\n");
				*db_version = g_version;
				*db_format = g_format;
				*db_flags = g_flags;
				if (!has_typed_quotas)
					fix_typed_quotas();
				if (g_format == F_MUX)
					fix_mux_zones();
				return mudstate.db_top;
			}
		default:
			fprintf(mainlog_fp, "\nIllegal character '%c' near object #%d\n",
				ch, i);
			return -1;
		}


	}
}
#endif /* STANDALONE */

int db_read()
{
	int *data, *dptr, len, vattr_flags, i;

#ifndef NO_TIMECHECKING
	struct timeval obj_time;
#endif	

	/* Fetch the database info */
	
	dddb_get((void *)"TM3", 4, (void **)&data, &len, DBTYPE_DBINFO);
	
	if (!data) {
		fprintf(mainlog_fp, "\nCould not open main record");
		return -1;
	}
	
	dptr = data;
	memcpy((void *)&mudstate.min_size, (void *)dptr, sizeof(int));
	dptr++;
	memcpy((void *)&mudstate.attr_next, (void *)dptr, sizeof(int));
	dptr++;
	memcpy((void *)&mudstate.record_players, (void *)dptr, sizeof(int));
	free(data);
	
	/* Load the attribute numbers */
	
	for (i = 0; i < mudstate.attr_next; i++) {
		dddb_get((void *)&i, sizeof(int), (void **)&data, &len,
			DBTYPE_ATRNUM);
		if (data) {
			dptr = data;	
			memcpy((void *)&vattr_flags, (void *)dptr, sizeof(int));
			dptr++;
			
			/* Make sure they're not marked dirty */
			vattr_flags &= ~AF_DIRTY;

			/* dptr now points to the beginning of the atr name */
			vattr_define((char *)dptr, i, vattr_flags);
			free(data);
		}
	}
	
	/* Load the object structures */
	
	for (i = 0; i < mudstate.min_size; i++) {
		dddb_get((void *)&i, sizeof(int), (void **)&data, &len,
			DBTYPE_OBJECT);
		if (data) {
			db_grow(i + 1);

			memcpy((void *)&(db[i]), (void *)data, sizeof(DUMPOBJ));
#ifndef NO_TIMECHECKING
			obj_time.tv_sec = obj_time.tv_usec = 0;
			s_Time_Used(i, obj_time);
#endif
			s_StackCount(i, 0);
			s_VarsCount(i, 0);
			s_StructCount(i, 0);
			s_InstanceCount(i, 0);

			/* check to see if it's a player */

			if (Typeof(i) == TYPE_PLAYER) {
				c_Connected(i);
			}
			
			free(data);
		}
	}	

#ifndef STANDALONE
	load_player_names();
#endif
	return (0);
}			

static int db_write_object(f, i, db_format, flags)
FILE *f;
dbref i;
int db_format, flags;
{
#ifndef STANDALONE
	ATTR *a;

#endif
	char *got, *as;
	dbref aowner;
	int ca, aflags, alen, save, j;
	BOOLEXP *tempbool;
	int *data;

	if (Going(i)) {
		if (flags & V_GDBM) {
			dddb_del(&i, sizeof(int), DBTYPE_OBJECT);
		}
		return (0);
	}
	
#ifndef STANDALONE
	if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
	if ((!(flags & V_GDBM))) {
#endif
		fprintf(f, "!%d\n", i);
		if (!(flags & V_ATRNAME))
			putstring(f, Name(i));
		putref(f, Location(i));
		if (flags & V_ZONE)
			putref(f, Zone(i));
		putref(f, Contents(i));
		putref(f, Exits(i));
		if (flags & V_LINK)
			putref(f, Link(i));
		putref(f, Next(i));
		if (!(flags & V_ATRKEY)) {
			got = atr_get(i, A_LOCK, &aowner, &aflags, &alen);
			tempbool = parse_boolexp(GOD, got, 1);
			free_lbuf(got);
			putboolexp(f, tempbool);
			if (tempbool)
				free_boolexp(tempbool);
		}
		putref(f, Owner(i));
		if (flags & V_PARENT)
			putref(f, Parent(i));
		if (!(flags & V_ATRMONEY))
			putref(f, Pennies(i));
		putref(f, Flags(i));
		if (flags & V_XFLAGS)
			putref(f, Flags2(i));
		if (flags & V_3FLAGS)
			putref(f, Flags3(i));
		if (flags & V_POWERS) {
			putref(f, Powers(i));
			putref(f, Powers2(i));
		}
		if (flags & V_TIMESTAMPS) {
			putlong(f, AccessTime(i));
			putlong(f, ModTime(i));
		}

		/* write the attribute list */

		for (ca = atr_head(i, &as); ca; ca = atr_next(&as)) {
			save = 0;
#ifndef STANDALONE
			a = atr_num(ca);
			if (a)
				j = a->number;
			else
				j = -1;
#else
			j = ca;
#endif

			if (j > 0) {
				switch (j) {
				case A_NAME:
					if (flags & V_ATRNAME)
						save = 1;
					break;
				case A_LOCK:
					if (flags & V_ATRKEY)
						save = 1;
					break;
				case A_LIST:
				case A_MONEY:
					break;
				default:
					save = 1;
				}
			}
			if (save) {
				got = atr_get_raw(i, j);
				fprintf(f, ">%d\n", j);
				putstring(f, got);
			}
		}
		fprintf(f, "<\n");
	} else {
		/* DBREF is our key */
		
		dddb_put((void *)&i, sizeof(int), (void *)&(db[i]),
			sizeof(DUMPOBJ), DBTYPE_OBJECT);
	}
	return 0;
}

dbref db_write(f, format, version)
FILE *f;
int format, version;
{
	dbref i;
	int flags;
	VATTR *vp;
	int *data, *dptr, len;

	/* If you're dumping to DBM then 'f' should be NULL since we don't
	 * use it */

#ifndef MEMORY_BASED
	al_store();
#endif /* MEMORY_BASED */

	switch (format) {
	case F_TINYMUSH:
		flags = version;
		break;
	default:
		fprintf(mainlog_fp, "Can only write TinyMUSH 3 format.\n");
		return -1;
	}
#ifdef STANDALONE
	fprintf(mainlog_fp, "Writing ");
#endif

	/* Write database information */

	i = mudstate.attr_next;

#ifndef STANDALONE
	if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
	if ((!(flags & V_GDBM))) {
#endif
		/* TinyMUSH 2 wrote '+V', MUX wrote '+X', 3.0 writes '+T'. */
		fprintf(f, "+T%d\n+S%d\n+N%d\n", flags, mudstate.db_top, i);
		fprintf(f, "-R%d\n", mudstate.record_players);
	} else {
		/* This should be the only data record of its type */
		
		dptr = data = (int *)malloc(3 * sizeof(int));
		memcpy((void *)dptr, (void *)&mudstate.db_top, sizeof(int));
		dptr++;
		memcpy((void *)dptr, (void *)&i, sizeof(int));
		dptr++;
		memcpy((void *)dptr, (void *)&mudstate.record_players, sizeof(int));
		
		/* "TM3" is our unique key */
		
		dddb_put((void *)"TM3", 4, (void *)data, (3 * sizeof(int)),
			DBTYPE_DBINFO);
		free(data);
	}
		

	/* Dump user-named attribute info */

	vp = vattr_first();
	while (vp != NULL) {
		if (!(vp->flags & AF_DELETED)) {
#ifndef STANDALONE
			if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
			if ((!(flags & V_GDBM))) {
#endif
				fprintf(f, "+A%d\n\"%d:%s\"\n",
					vp->number, vp->flags, vp->name);
			} else if (vp->flags & AF_DIRTY) {
				/* Only write the dirty attribute numbers
				 * and clear the flag */
				 
				vp->flags &= ~AF_DIRTY;
				len = strlen(vp->name) + 1;
				dptr = data = (int *)malloc(sizeof(int) + len);
				memcpy((void *)dptr, (void *)&vp->flags, sizeof(int));
				dptr++;
				memcpy((void *)dptr, (void *)vp->name, len); 
				
				/* Attribute number is our key */
				
				dddb_put((void *)&vp->number, sizeof(int),
					data, sizeof(int) + len, DBTYPE_ATRNUM);
				free(data);
			}
		} else {
#ifndef STANDALONE
			if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
			if ((!(flags & V_GDBM))) {
#endif
				/* Delete the attribute number entry */
				dddb_del((void *)vp->number, sizeof(int),
					DBTYPE_ATRNUM);
			}
		}
		vp = vattr_next(vp);
	}

	DO_WHOLE_DB(i) {

#ifdef STANDALONE
		if (!(i % 100)) {
			fputc('.', mainlog_fp);
		}
#endif

		db_write_object(f, i, format, flags);
	}

#ifndef STANDALONE
	if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
	if ((!(flags & V_GDBM))) {
#endif
		fputs("***END OF DUMP***\n", f);
		fflush(f);
	}
	
#ifdef STANDALONE
	fprintf(mainlog_fp, "\n");
#endif
	return (mudstate.db_top);
}
