/* db_rw.c */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <sys/file.h>

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "powers.h"

extern const char *FDECL(getstring_noalloc, (FILE *, int));
extern void FDECL(putstring, (FILE *, const char *));
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
	        fprintf(stderr, "ABORT! db_rw.c, unexpected EOF in boolexp in getboolexp1().\n");
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
			fprintf(stderr, "ABORT! db_rw.c, unexpected EOF in getboolexp1().\n");
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
			buff = alloc_lbuf("getboolexp1.attr_lock");
			StringCopy(buff, getstring_noalloc(f, 1));
			b->sub1 = (BOOLEXP *) strsave(buff);
			free_lbuf(buff);
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
			b->sub1 = (BOOLEXP *) strsave(buff);
			free_lbuf(buff);
		}
		ungetc(c, f);
		return b;
	}

      error:
	fprintf(stderr,
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
	    fprintf(stderr, "ABORT! db_rw.c, parse error in getboolexp().\n");
	    abort();	/* parse error, we lose */
	}

	/* MUSH (except for PernMUSH) and MUSE can have an extra CR, MUD
	 * does not. 
	 */

	if (((g_format == F_MUSH) && (g_version != 2)) ||
	    (g_format == F_MUSE) || (g_format == F_MUX) ||
	    (g_format == F_TINYMUSH)) {
		if ((c = getc(f)) != '\n')
			ungetc(c, f);
	}
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
	case F_MUSE:
		switch (attrnum) {
		case 39:
			return A_IDLE;
		case 40:
			return A_AWAY;
		case 41:
			return 0;	/* mailk */
		case 42:
			return A_ALIAS;
		case 43:
			return A_EFAIL;
		case 44:
			return A_OEFAIL;
		case 45:
			return A_AEFAIL;
		case 46:
			return 0;	/* it */
		case 47:
			return A_LEAVE;
		case 48:
			return A_OLEAVE;
		case 49:
			return A_ALEAVE;
		case 50:
			return 0;	/* channel */
		case 51:
			return A_QUOTA;
		case 52:
			return A_TEMP;	/* temp for pennies */
		case 53:
			return 0;	/* huhto */
		case 54:
			return 0;	/* haven */
		case 57:
			return mkattr((char *)"TZ");
		case 58:
			return 0;	/* doomsday */
		case 59:
			return mkattr((char *)"Email");
		case 98:
			return mkattr((char *)"Status");
		case 99:
			return mkattr((char *)"Race");
		default:
			return attrnum;
		}
	case F_MUSH:

	    if (g_version != 2) {

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

	    } else {

		/* Penn variant */

		switch (attrnum) {
		case 34:
			return A_OENTER;
		case 41:
			return A_LEAVE;
		case 42:
			return A_ALEAVE;
		case 43:
			return A_OLEAVE;
		case 44:
			return A_OXENTER;
		case 45:
			return A_OXLEAVE;
		default:
			if ((attrnum >= 126) && (attrnum < 152)) {
				anam[0] = 'W';
				anam[1] = attrnum - 126 + 'A';
				anam[2] = '\0';
				return mkattr(anam);
			}
			if ((attrnum >= 152) && (attrnum < 178)) {
				anam[0] = 'X';
				anam[1] = attrnum - 152 + 'A';
				anam[2] = '\0';
				return mkattr(anam);
			}
			return attrnum;
		}
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
#ifdef STANDALONE
		case ']':	/* Pern 1.13 style text attribute */
			strcpy(buff, (char *)getstring_noalloc(f, new_strings));

			/* Get owner number */

			ownp = (char *)index(buff, '^');
			if (!ownp) {
				fprintf(stderr,
				   "Bad format in attribute on object %d\n",
					i);
				free_lbuf(buff);
				return 0;
			}
			*ownp++ = '\0';

			/* Get attribute flags */

			flagp = (char *)index(ownp, '^');
			if (!flagp) {
				fprintf(stderr,
				   "Bad format in attribute on object %d\n",
					i);
				free_lbuf(buff);
				return 0;
			}
			*flagp++ = '\0';

			/* Convert Pern-style owner and flags to 2.0 format */

			aowner = atoi(ownp);
			xflags = atoi(flagp);
			aflags = 0;

			if (!aowner)
				aowner = NOTHING;
			if (xflags & 0x10)
				aflags |= AF_LOCK | AF_NOPROG;
			if (xflags & 0x20)
				aflags |= AF_NOPROG;

			if (!strcmp(buff, "XYXXY"))
				s_Pass(i, (char *)getstring_noalloc(f, new_strings));
			else {
				/* Look up the attribute name in the
				 * attribute table. If the name isn't
				 * found, create a new attribute. If the
				 * create fails, try prefixing the attr
				 * name with ATR_ (Pern allows attributes 
				 * to start with a non-alphabetic
				 * character. 
				 */

				anum = mkattr(buff);
				if (anum < 0) {
					buf2 = alloc_mbuf("get_list.new_attr_name");
					buf2p = buf2;
					safe_mb_str((char *)"ATR_", buf2, &buf2p);
					safe_mb_str(buff, buf2, &buf2p);
					*buf2p = '\0';
					anum = mkattr(buf2);
					free_mbuf(buf2);
				}
				/*
				 * MAILFOLDERS under MUSH must be owned by the 
				 * player, not GOD 
				 */

				if (!strcmp(buff, "MAILFOLDERS")) {
					aowner = Owner(i);
				}
				if (anum < 0) {
					fprintf(stderr,
						"Bad attribute name '%s' on object %d, ignoring...\n",
						buff, i);
					(void)getstring_noalloc(f, new_strings);
				} else {
					atr_add(i, anum, (char *)getstring_noalloc(f, new_strings),
						aowner, aflags);
				}
			}
			break;
#endif
		case '\n':	/* ignore newlines. They're due to v(r). */
			break;
		case '<':	/* end of list */
			free_lbuf(buff);
			c = getc(f);
			if (c != '\n') {
				ungetc(c, f);
				fprintf(stderr,
					"No line feed on object %d\n", i);
				return 1;
			}
			return 1;
		default:
			fprintf(stderr,
				"Bad character '%c' when getting attributes on object %d\n",
				c, i);
			/* We've found a bad spot.  I hope things aren't
			 * too bad. 
			 */

			(void)getstring_noalloc(f, new_strings);
		}
	}
	return 1;		/* UNREACHED */
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
		fprintf(stderr, "Unknown boolean type in putbool_subexp: %d\n",
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
	if (db_format == F_MUD) {

		/* Old TinyMUD format */

		newf1 = f1 & (TYPE_MASK | WIZARD | LINK_OK | DARK | STICKY | HAVEN);
		if (f1 & MUD_ABODE)
			newf2 |= ABODE;
		if (f1 & MUD_ROBOT)
			newf1 |= ROBOT;
		if (f1 & MUD_CHOWN_OK)
			newf1 |= CHOWN_OK;

	} else if (db_format == F_MUSE) {
		if (db_version == 1)
			return;

		/* Convert level-based players to normal */

		switch (f1 & 0xf) {
		case 0:	/* room */
		case 1:	/* thing */
		case 2:	/* exit  */
			newf1 = f1 & 0x3;
			break;
		case 8:	/* guest */
		case 9:	/* trial player */
		case 10:	/* member */
		case 11:	/* junior official */
		case 12:	/* official */
			newf1 = TYPE_PLAYER;
			break;
		case 13:	/* honorary wizard */
		case 14:	/* administrator */
		case 15:	/* director */
			newf1 = TYPE_PLAYER | WIZARD;
			break;
		default:	/* A bad type, mark going */
			fprintf(stderr, "Funny object type for #%d\n", thing);
			*flags1 = GOING;
			return;
		}

		/* Player #1 is always a wizard */

		if (thing == (dbref) 1)
			newf1 |= WIZARD;

		/* Set type-specific flags */

		switch (newf1 & TYPE_MASK) {
		case TYPE_PLAYER:	/* Lose CONNECT TERSE QUITE NOWALLS
					 * WARPTEXT 
					 */
			if (f1 & MUSE_BUILD)
				s_Powers(thing, Powers(thing) | POW_BUILDER);
			if (f1 & MUSE_SLAVE)
				newf2 |= SLAVE;
			if (f1 & MUSE_UNFIND)
				newf2 |= UNFINDABLE;
			break;
		case TYPE_THING:	/* lose LIGHT SACR_OK */
			if (f1 & MUSE_KEY)
				newf2 |= KEY;
			if (f1 & MUSE_DEST_OK)
				newf1 |= DESTROY_OK;
			break;
		case TYPE_ROOM:
			if (f1 & MUSE_ABODE)
				newf2 |= ABODE;
			break;
		case TYPE_EXIT:
			if (f1 & MUSE_SEETHRU)
				newf1 |= SEETHRU;
		default:
			break;
		}

		/* Convert common flags */
		/* Lose: MORTAL ACCESSED MARKED SEE_OK UNIVERSAL */

		if (f1 & MUSE_CHOWN_OK)
			newf1 |= CHOWN_OK;
		if (f1 & MUSE_DARK)
			newf1 |= DARK;
		if (f1 & MUSE_STICKY)
			newf1 |= STICKY;
		if (f1 & MUSE_HAVEN)
			newf1 |= HAVEN;
		if (f1 & MUSE_INHERIT)
			newf1 |= INHERIT;
		if (f1 & MUSE_GOING)
			newf1 |= GOING;
		if (f1 & MUSE_PUPPET)
			newf1 |= PUPPET;
		if (f1 & MUSE_LINK_OK)
			newf1 |= LINK_OK;
		if (f1 & MUSE_ENTER_OK)
			newf1 |= ENTER_OK;
		if (f1 & MUSE_VISUAL)
			newf1 |= VISUAL;
		if (f1 & MUSE_OPAQUE)
			newf1 |= OPAQUE;
		if (f1 & MUSE_QUIET)
			newf1 |= QUIET;

	} else if ((db_format == F_MUSH) && (db_version == 2)) {

		/* Pern variants */

		newf1 = (f1 & TYPE_MASK);
		newf2 = 0;

		newf1 &= ~PENN_COMBAT;
		newf1 &= ~PENN_ACCESSED;
		newf1 &= ~PENN_MARKED;
		newf1 &= ~ROYALTY;

		if (f1 & PENN_INHERIT)
			newf1 |= INHERIT;
		if (f1 & PENN_AUDIBLE)
			newf1 |= HEARTHRU;
		if (f1 & PENN_ROYALTY)
			newf1 |= ROYALTY;
		if (f1 & PENN_WIZARD)
			newf1 |= WIZARD;
		if (f1 & PENN_LINK_OK)
			newf1 |= LINK_OK;
		if (f1 & PENN_DARK)
			newf1 |= DARK;
		if (f1 & PENN_VERBOSE)
			newf1 |= VERBOSE;
		if (f1 & PENN_STICKY)
			newf1 |= STICKY;
		if (f1 & PENN_TRANSPARENT)
			newf1 |= SEETHRU;
		if (f1 & PENN_HAVEN)
			newf1 |= HAVEN;
		if (f1 & PENN_QUIET)
			newf1 |= QUIET;
		if (f1 & PENN_HALT)
			newf1 |= HALT;
		if (f1 & PENN_UNFIND)
			newf2 |= UNFINDABLE;
		if (f1 & PENN_GOING)
			newf1 |= GOING;
		if (f1 & PENN_CHOWN_OK)
			newf1 |= CHOWN_OK;
		if (f1 & PENN_ENTER_OK)
			newf1 |= ENTER_OK;
		if (f1 & PENN_VISUAL)
			newf1 |= VISUAL;
		if (f1 & PENN_OPAQUE)
			newf1 |= OPAQUE;
		if (f1 & PENN_DEBUGGING)
			newf1 |= TRACE;
		if (f1 & PENN_SAFE)
			newf1 |= SAFE;
		if (f1 & PENN_STARTUP)
			newf1 |= HAS_STARTUP;

		switch (newf1 & TYPE_MASK) {
		case TYPE_PLAYER:
			if (f2 & PENN_PLAYER_TERSE)
				newf1 |= TERSE;
			if (f2 & PENN_PLAYER_MYOPIC)
				newf1 |= MYOPIC;
			if (f2 & PENN_PLAYER_NOSPOOF)
				newf1 |= NOSPOOF;
			if (f2 & PENN_PLAYER_SUSPECT)
				newf2 |= SUSPECT;
			if (f2 & PENN_PLAYER_GAGGED)
				newf2 |= SLAVE;
			if (f2 & PENN_PLAYER_MONITOR)
				newf1 |= MONITOR;
			if (f2 & PENN_PLAYER_CONNECT)
				newf2 &= ~CONNECTED;
			if (f2 & PENN_PLAYER_ANSI)
				newf2 |= ANSI;
			if (f2 & PENN_PLAYER_HEAD)
				newf2 |= HEAD_FLAG;
			if (f2 & PENN_PLAYER_FIXED)
				newf2 |= FIXED;
			if (f2 & PENN_PLAYER_ADMIN)
				newf2 |= STAFF;
			if (f2 & PENN_PLAYER_SLAVE)
				newf2 |= SLAVE;
			if (f2 & PENN_PLAYER_COLOR)
				newf2 |= ANSI;
			if (f2 & PENN_PLAYER_WEIRDANSI)
				newf2 |= NOBLEED;
			break;
		case TYPE_EXIT:
			if (f2 & PENN_EXIT_LIGHT)
				newf2 |= LIGHT;
			break;
		case TYPE_THING:
			if (f2 & PENN_THING_DEST_OK)
				newf1 |= DESTROY_OK;
			if (f2 & PENN_THING_PUPPET)
				newf1 |= PUPPET;
			if (f2 & PENN_THING_LISTEN)
				newf1 |= MONITOR;
			break;
		case TYPE_ROOM:
			if (f2 & PENN_ROOM_FLOATING)
				newf2 |= FLOATING;
			if (f2 & PENN_ROOM_ABODE)
				newf2 |= ABODE;
			if (f2 & PENN_ROOM_JUMP_OK)
				newf1 |= JUMP_OK;
			if (f2 & PENN_ROOM_LISTEN)
				newf1 |= MONITOR;
			if (f2 & PENN_ROOM_UNINSPECT)
				newf2 |= UNINSPECTED;
		}
	} else if ((db_format == F_MUSH) && (db_version >= 3)) {
		newf1 = f1;
		newf2 = f2;
		newf3 = 0;
		switch (db_version) {
		case 3:
			(newf1 &= ~V2_ACCESSED);	/* Clear ACCESSED */
		case 4:
			(newf1 &= ~V3_MARKED);	/* Clear MARKED */
		case 5:
			/* Merge GAGGED into SLAVE, move SUSPECT */

			if ((newf1 & TYPE_MASK) == TYPE_PLAYER) {
				if (newf1 & V4_GAGGED) {
					newf2 |= SLAVE;
					newf1 &= ~V4_GAGGED;
				}
				if (newf1 & V4_SUSPECT) {
					newf2 |= SUSPECT;
					newf1 &= ~V4_SUSPECT;
				}
			}
		case 6:
			switch (newf1 & TYPE_MASK) {
			case TYPE_PLAYER:
				if (newf1 & V6_BUILDER) {
					s_Powers(thing, Powers(thing) | POW_BUILDER);
					newf1 &= ~V6_BUILDER;
				}
				if (newf1 & V6_SUSPECT) {
					newf2 |= SUSPECT;
					newf1 &= ~V6_SUSPECT;
				}
				if (newf1 & V6PLYR_UNFIND) {
					newf2 |= UNFINDABLE;
					newf1 &= ~V6PLYR_UNFIND;
				}
				if (newf1 & V6_SLAVE) {
					newf2 |= SLAVE;
					newf1 &= ~V6_SLAVE;
				}
				break;
			case TYPE_ROOM:
				if (newf1 & V6_FLOATING) {
					newf2 |= FLOATING;
					newf1 &= ~V6_FLOATING;
				}
				if (newf1 & V6_ABODE) {
					newf2 |= ABODE;
					newf1 &= ~V6_ABODE;
				}
				if (newf1 & V6ROOM_JUMPOK) {
					newf1 |= JUMP_OK;
					newf1 &= ~V6ROOM_JUMPOK;
				}
				if (newf1 & V6ROOM_UNFIND) {
					newf2 |= UNFINDABLE;
					newf1 &= ~V6ROOM_UNFIND;
				}
				break;
			case TYPE_THING:
				if (newf1 & V6OBJ_KEY) {
					newf2 |= KEY;
					newf1 &= ~V6OBJ_KEY;
				}
				break;
			case TYPE_EXIT:
				if (newf1 & V6EXIT_KEY) {
					newf2 |= KEY;
					newf1 &= ~V6EXIT_KEY;
				}
				break;
			}
		}

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
		if (newf2 & PLACEHOLDER) {
		    newf2 &= ~PLACEHOLDER;
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

	    if (newf2 & PLACEHOLDER) {
		/* This used to be the COMPRESS flag, which didn't do
		 * anything, and still doesn't do anything.
		 */
		newf2 &= ~PLACEHOLDER;
	    }

	} else if (db_format == F_TINYMUSH) {
		newf1 = f1;
		newf2 = f2;
		newf3 = f3;
	}
	*flags1 = newf1;
	*flags2 = newf2;
	*flags3 = newf3;
	return;
}


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

    zmarks = (int *) calloc(mudstate.db_top, sizeof(int));

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

    free(zmarks);
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

#endif /* STANDALONE */


/* ---------------------------------------------------------------------------
 * unscraw_foreign: Fix up strange object linking conventions for other formats
 */

void unscraw_foreign(db_format, db_version, db_flags)
int db_format, db_version, db_flags;
{
	dbref tmp, i, aowner;
	int aflags, alen;
	char *p_str;

	switch (db_format) {
	case F_MUSE:
		DO_WHOLE_DB(i) {
			if (Typeof(i) == TYPE_EXIT) {

				/* MUSE exits are bass-ackwards */

				tmp = Exits(i);
				s_Exits(i, Location(i));
				s_Location(i, tmp);
			}
			if (db_version > 3) {

				/* MUSEs with pennies in an attribute have 
				 * it stored in attr 255 (see 
				 * unscramble_attrnum) 
				 */

				p_str = atr_get(i, A_TEMP, &aowner, &aflags, &alen);
				s_Pennies(i, atoi(p_str));
				free_lbuf(p_str);
				atr_clr(i, A_TEMP);
			}
		}
		if (!(db_flags & V_LINK)) {
			efo_convert();
		}
		break;
	case F_MUSH:
		if ((db_version <= 5) && (db_flags & V_GDBM)) {

			/* Check for FORWARDLIST attribute */

			DO_WHOLE_DB(i) {
				if (atr_get_raw(i, A_FORWARDLIST))
					s_Flags2(i, Flags2(i) | HAS_FWDLIST);
			}
		}
		if (db_version <= 6) {
			DO_WHOLE_DB(i) {

				/* Make sure A_QUEUEMAX is empty */

				atr_clr(i, A_QUEUEMAX);

				if (db_flags & V_GDBM) {

					/* HAS_LISTEN now tracks LISTEN attr */

					if (atr_get_raw(i, A_LISTEN))
						s_Flags2(i, Flags2(i) | HAS_LISTEN);

					/* Undo V6 overloading of HAS_STARTUP
					 * with HAS_FWDLIST 
					 */

					if ((db_version == 6) &&
					    (Flags2(i) & HAS_STARTUP) &&
					    atr_get_raw(i, A_FORWARDLIST)) {

						/* We have FORWARDLIST */

						s_Flags2(i, Flags2(i) | HAS_FWDLIST);

						/* Maybe no STARTUP */

						if (!atr_get_raw(i, A_STARTUP))
							s_Flags2(i, Flags2(i) & ~HAS_STARTUP);
					}
				}
			}
		}
		break;
	case F_MUD:
		efo_convert();
	}
}

/* ---------------------------------------------------------------------------
 * getlist_discard, get_atrdefs_discard: Throw away data from MUSE that we
 * don't use.
 */

static void getlist_discard(f, i, set)
FILE *f;
dbref i;
int set;
{
	int count;

	count = getref(f);
	for (count--; count >= 0; count--) {
		if (set)
			s_Parent(i, getref(f));
		else
			(void)getref(f);
	}
}

static void get_atrdefs_discard(f)
FILE *f;
{
	const char *sp;

	for (;;) {
		sp = getstring_noalloc(f, 0);	/* flags or endmarker */
		if (*sp == '\\')
			return;
		sp = getstring_noalloc(f, 0);	/* object */
		sp = getstring_noalloc(f, 0);	/* name */
	}
}

static void getpenn_new_locks(f, i)
    FILE *f;
    dbref i;
{
	int c;
	char *buf, *p;
	struct boolexp *tempbool;

	buf = alloc_lbuf("getpenn_new_locks");
	p = buf;
	while (c = getc(f), (c != EOF) && (c != '|'))
		*p++ = c;

	*p = '\0';
	
	tempbool = getboolexp(f);
	
	if (tempbool == TRUE_BOOLEXP)
		return;
		
	if (!strcmp(buf, "Basic"))
		atr_add_raw(i, A_LOCK,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Use"))
		atr_add_raw(i, A_LUSE,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Enter"))
		atr_add_raw(i, A_LENTER,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Page"))
		atr_add_raw(i, A_LPAGE,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Teleport"))
		atr_add_raw(i, A_LTPORT,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Speech"))
		atr_add_raw(i, A_LSPEECH,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Parent"))
		atr_add_raw(i, A_LPARENT,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Link"))
		atr_add_raw(i, A_LLINK,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Leave"))
		atr_add_raw(i, A_LLEAVE,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Drop"))
		atr_add_raw(i, A_LDROP,
			    unparse_boolexp_quiet(1, tempbool));
	else if (!strcmp(buf, "Give"))
		atr_add_raw(i, A_LGIVE,
			    unparse_boolexp_quiet(1, tempbool));
	free_lbuf(buf);
}
#endif 

dbref db_read(f, db_format, db_version, db_flags)
FILE *f;
int *db_format, *db_version, *db_flags;
{
	dbref i, anum;
	char ch;
	const char *tstr;
	int header_gotten, size_gotten, nextattr_gotten;
	int read_attribs, read_name, read_zone, read_link, read_key, read_parent;
	int read_extflags, read_3flags, read_money, read_timestamps, read_new_strings;
#ifdef STANDALONE
	int is_penn, read_pern_key, read_pern_comm, read_pern_parent;
	int read_pern_warnings, read_pern_creation, read_pern_powers;
	int read_pern_new_locks, is_dark;
	int read_dark_comm, read_dark_slock, read_dark_mc, read_dark_mpar;
	int read_dark_class, read_dark_rank, read_dark_droplock;
	int read_dark_givelock, read_dark_getlock;
	int read_dark_threepow, penn_version;
	int read_muse_parents, read_muse_atrdefs;
	char peek;
#endif
	int read_powers, read_powers_player, read_powers_any;
	int has_typed_quotas;
	int deduce_version, deduce_name, deduce_zone, deduce_timestamps;
	int aflags, f1, f2, f3;
	BOOLEXP *tempbool;
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
	read_timestamps = 0;
	read_new_strings = 0;
	read_powers = 0;
	read_powers_player = 0;
	read_powers_any = 0;
	deduce_version = 1;
	deduce_zone = 1;
	deduce_name = 1;
	deduce_timestamps = 1;
#ifdef STANDALONE
	is_penn = 0;
	is_dark = 0;
	read_pern_key = 0;
	read_pern_comm = 0;
	read_pern_parent = 0;
	read_pern_warnings = 0;
	read_pern_creation = 0;
	read_pern_powers = 0;
	read_pern_new_locks = 0;
	read_dark_comm = 0;
	read_dark_slock = 0;
	read_dark_mc = 0;
	read_dark_mpar = 0;
	read_dark_class = 0;
	read_dark_rank = 0;
	read_dark_droplock = 0;
	read_dark_givelock = 0;
	read_dark_getlock = 0;
	read_dark_threepow = 0;
	penn_version = 0;
	read_muse_parents = 0;
	read_muse_atrdefs = 0;

	fprintf(stderr, "Reading ");
	fflush(stderr);
#endif
	db_free();
	for (i = 0;; i++) {

#ifndef MEMORY_BASED
		if (!(i % 25)) {
			cache_reset(0);
		}
#endif /* MEMORY_BASED */
#ifdef STANDALONE
		if (!(i % 100)) {
			fputc('.', stderr);
			fflush(stderr);
		}
#endif

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
#ifdef STANDALONE
		case '~':	/* Database size tag */
			is_penn = 1;
			if (!is_dark && penn_version) {
				g_format = F_MUSH;
				g_version = (((penn_version - 2) / 256) - 5);
				/* Okay, let's try and unscraw version
				 * encoding method they use in later Penn 
				 * 1.50. 
				 */

				/* Handle Pern veriants specially */

				if (g_version & 0x20)
					read_new_strings = 1;

				if (g_version & 0x10)
					read_pern_new_locks = 1;

				if (!(g_version & 0x08))
					read_pern_powers = 1;

				if (g_version & 0x04)
					read_pern_creation = 1;

				if (g_version & 0x02)
					read_pern_warnings = 1;

				if (!(g_version & 0x01))
					read_pern_comm = 1;

				g_version = 2;
			}
			if (size_gotten) {
				fprintf(stderr,
					"\nDuplicate size entry at object %d, ignored.\n",
					i);
				tstr = getstring_noalloc(f, 0);	/* junk */
				break;
			}
			mudstate.min_size = getref(f);
			size_gotten = 1;
			break;
#endif
		case '+':	/* MUX and MUSH header */

		    ch = getc(f); /* 2nd char selects type */

		    if ((ch == 'V') || (ch == 'X') || (ch == 'T')) {

			/* The following things are common across 2.x, MUX,
			 * and 3.0.
			 */
		        
			 if (header_gotten) {
			     fprintf(stderr,
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

#ifdef STANDALONE
			case 'V':	/* 2.0 VERSION */
			    g_format = F_MUSH;
			    penn_version = g_version;
			    g_version &= V_MASK;
			    break;

			case 'X':	/* MUX VERSION */
			    g_format = F_MUX;
			    read_3flags = (g_version & V_3FLAGS);
			    read_powers = (g_version & V_POWERS);
			    read_new_strings = (g_version & V_QUOTED);
			    g_version &= V_MASK;
			    break;

			case 'K':	/* Kalkin's DarkZone dist */
				/* Him and his #defines... */
				if (header_gotten) {
					fprintf(stderr,
						"\nDuplicate MUSH version header entry at object %d, ignored.\n",
						i);
					tstr = getstring_noalloc(f, 0);
					break;
				}
				header_gotten = 1;
				deduce_version = 0;
				g_format = F_MUSH;
				g_version = getref(f);
				is_dark = 1;
				is_penn = 1;

				read_pern_powers = 1;
				if (g_version & DB_CHANNELS)
					read_dark_comm = 1;
				if (g_version & DB_SLOCK)
					read_dark_slock = 1;
				if (g_version & DB_MC)
					read_dark_mc = 1;
				if (g_version & DB_MPAR)
					read_dark_mpar = 1;
				if (g_version & DB_CLASS)
					read_dark_class = 1;
				if (g_version & DB_RANK)
					read_dark_rank = 1;
				if (g_version & DB_DROPLOCK)
					read_dark_droplock = 1;
				if (g_version & DB_GIVELOCK)
					read_dark_givelock = 1;
				if (g_version & DB_GETLOCK)
					read_dark_getlock = 1;
				if (g_version & DB_THREEPOW)
					read_dark_threepow = 1;
				g_version = 2;
				break;
#endif /* STANDALONE */
			case 'S':	/* SIZE */
				if (size_gotten) {
					fprintf(stderr,
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
					fprintf(stderr,
						"\nDuplicate next free vattr entry at object %d, ignored.\n",
						i);
					tstr = getstring_noalloc(f, 0);
				} else {
					mudstate.attr_next = getref(f);
					nextattr_gotten = 1;
				}
				break;
			default:
				fprintf(stderr,
					"\nUnexpected character '%c' in MUSH header near object #%d, ignored.\n",
					ch, i);
				tstr = getstring_noalloc(f, 0);
			}
			break;
#ifdef STANDALONE
		case '@':	/* MUSE header */
			if (header_gotten) {
				fprintf(stderr,
					"\nDuplicate MUSE header entry at object #%d.\n",
					i);
				return -1;
			}
			header_gotten = 1;
			deduce_version = 0;
			g_format = F_MUSE;
			g_version = getref(f);
			deduce_name = 0;
			deduce_zone = 1;
			read_money = (g_version <= 3);
			read_link = (g_version >= 5);
			read_powers_player = (g_version >= 6);
			read_powers_any = (g_version == 6);
			read_muse_parents = (g_version >= 8);
			read_muse_atrdefs = (g_version >= 8);
			if (read_link)
				g_flags |= V_LINK;
			break;
		case '#':
			if (deduce_version) {
				g_format = F_MUD;
				g_version = 1;
				deduce_version = 0;
			}
			if (g_format != F_MUD) {
				fprintf(stderr,
					"\nMUD-style object found in non-MUD database at object #%d\n",
					i);
				return -1;
			}
			if (i != getref(f)) {
				fprintf(stderr,
				     "\nSequence error at object #%d\n", i);
				return -1;
			}
			db_grow(i + 1);
			s_Name(i, (char *)getstring_noalloc(f, 0));
			atr_add_raw(i, A_DESC, (char *)getstring_noalloc(f, 0));
			s_Location(i, getref(f));
			s_Contents(i, getref(f));
			s_Exits(i, getref(f));
			s_Link(i, NOTHING);
			s_Next(i, getref(f));
#ifndef NO_TIMECHECKING
			obj_time.tv_sec = obj_time.tv_usec = 0;
			s_Time_Used(i, obj_time);
#endif
			s_StackCount(i, 0);
			s_VarsCount(i, 0);
			s_StructCount(i, 0);
			s_InstanceCount(i, 0);
/* s_Zone(i, NOTHING); */
			tempbool = getboolexp(f);
			atr_add_raw(i, A_LOCK,
				    unparse_boolexp_quiet(1, tempbool));
			free_boolexp(tempbool);
			atr_add_raw(i, A_FAIL, (char *)getstring_noalloc(f, 0));
			atr_add_raw(i, A_SUCC, (char *)getstring_noalloc(f, 0));
			atr_add_raw(i, A_OFAIL, (char *)getstring_noalloc(f, 0));
			atr_add_raw(i, A_OSUCC, (char *)getstring_noalloc(f, 0));
			s_Owner(i, getref(f));
			s_Parent(i, NOTHING);
			s_Pennies(i, getref(f));
			f1 = getref(f);
			f2 = 0;
			f3 = 0;
			upgrade_flags(&f1, &f2, &f3, i, g_format, g_version);
			s_Flags(i, f1);
			s_Flags2(i, f2);
			s_Flags3(i, f3);
			s_Pass(i, getstring_noalloc(f, 0));
			if (deduce_timestamps) {
				peek = getc(f);
				if ((peek != '#') && (peek != '*')) {
					read_timestamps = 1;
				}
				deduce_timestamps = 0;
				ungetc(peek, f);
			}
			if (read_timestamps) {
				aflags = getref(f);	/* created */
				aflags = getref(f);	/* lastused */
				aflags = getref(f);	/* usecount */
			}
			break;
		case '&':	/* MUSH 2.0a stub entry/MUSE zoned entry */
			if (deduce_version) {
				deduce_version = 0;
				g_format = F_MUSH;
				g_version = 1;
				deduce_name = 1;
				deduce_zone = 0;
				read_key = 0;
				read_attribs = 0;
			} else if (deduce_zone) {
				deduce_zone = 0;
				read_zone = 1;
				g_flags |= V_ZONE;
			}
#endif
		case '!':	/* MUX entry/MUSH entry/MUSE non-zoned entry */
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

#ifdef STANDALONE
			if (is_penn) {
				tstr = getstring_noalloc(f, read_new_strings);
				s_Name(i, (char *)tstr);
				s_Location(i, getref(f));
				s_Contents(i, getref(f));
				s_Exits(i, getref(f));
				s_Next(i, getref(f));
				/* have no equivalent to multi-parents yet,
				 * so we have to throw
				 * them away... 
				 */

				if (read_dark_mpar) {
					/* Parents */
					getlist_discard(f, i, 1);
					/* Children */
					getlist_discard(f, i, 0);
				} else {
					s_Parent(i, getref(f));
				}

				if (read_pern_new_locks) {
					while ((ch = getc(f)) == '_')
						getpenn_new_locks(f, i);
					ungetc(ch, f);
				} else {
					tempbool = getboolexp(f);
					atr_add_raw(i, A_LOCK,
					unparse_boolexp_quiet(1, tempbool));
					free_boolexp(tempbool);
					tempbool = getboolexp(f);
					atr_add_raw(i, A_LUSE,
					unparse_boolexp_quiet(1, tempbool));
					free_boolexp(tempbool);
					tempbool = getboolexp(f);
					atr_add_raw(i, A_LENTER,
					unparse_boolexp_quiet(1, tempbool));
					free_boolexp(tempbool);
				}

				if (read_dark_slock)
					(void)getref(f);

				if (read_dark_droplock)
					(void)getref(f);

				if (read_dark_givelock)
					(void)getref(f);

				if (read_dark_getlock)
					(void)getref(f);

				s_Owner(i, getref(f));
				s_Zone(i, getref(f));
				s_Pennies(i, getref(f));
				f1 = getref(f);
				f2 = getref(f);
				f3 = 0;
				upgrade_flags(&f1, &f2, &f3, i, F_MUSH, 2);

				s_Flags(i, f1);
				s_Flags2(i, f2);
				s_Flags3(i, f3);

				if (read_pern_powers)
					(void)getref(f);

				/* Kalkin's extra two powers words... */
				if (read_dark_threepow) {
					(void)getref(f);
					(void)getref(f);
				}
				/* Kalkin's @class */
				if (read_dark_class)
					(void)getref(f);

				/* Kalkin put his creation times BEFORE
				 * channels unlike standard Penn... 
				 */

				if (read_dark_mc) {
					(void)getref(f);
					(void)getref(f);
				}
				if (read_pern_comm || read_dark_comm)
					(void)getref(f);

				if (read_pern_warnings)
					(void)getref(f);

				if (read_pern_creation) {
					(void)getref(f);
					(void)getref(f);
				}
				/* In Penn, clear the player's parent. */
				if (isPlayer(i)) {
					s_Parent(i, NOTHING);
				}
				if (!get_list(f, i, read_new_strings)) {
					fprintf(stderr,
						"\nError reading attrs for object #%d\n",
						i);
					return -1;
				}
			} else {
#endif
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

				/* ZONE on MUSE databases and some others */

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

				/* PARENT: PennMUSH uses this field for ZONE
				 * (which we  use as PARENT if we
				 * didn't already read in a  
				 * non-NOTHING parent. 
				 */

				if (read_parent) {
					s_Parent(i, getref(f));
				} else {
					s_Parent(i, NOTHING);
				}

				/* PENNIES */

				if (read_money)		/* if not fix in
							 * unscraw_foreign  
							 */
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

#ifdef STANDALONE					
				upgrade_flags(&f1, &f2, &f3, i, g_format, g_version);
#endif
				s_Flags(i, f1);
				s_Flags2(i, f2);
				s_Flags3(i, f3);


#ifdef STANDALONE
				/* POWERS from MUSE.  Discard. */

				if (read_powers_any ||
				    ((Typeof(i) == TYPE_PLAYER) && read_powers_player))
					(void)getstring_noalloc(f, 0);
#endif

				if (read_powers) {
					f1 = getref(f);
					f2 = getref(f);
					s_Powers(i, f1);
					s_Powers2(i, f2);
				}
				
				/* ATTRIBUTES */

				if (read_attribs) {
					if (!get_list(f, i, read_new_strings)) {
						fprintf(stderr,
							"\nError reading attrs for object #%d\n",
							i);
						return -1;
					}
				}

#ifdef STANDALONE
				/* PARENTS from MUSE.  Ewwww. */

				if (read_muse_parents) {
					getlist_discard(f, i, 1);
					getlist_discard(f, i, 0);
				}
				/* ATTRIBUTE DEFINITIONS from MUSE.  Ewwww.
				 * Ewwww. 
				 */

				if (read_muse_atrdefs) {
					get_atrdefs_discard(f);
				}

#endif
				/* check to see if it's a player */

				if (Typeof(i) == TYPE_PLAYER) {
					c_Connected(i);
				}
#ifdef STANDALONE
			}
#endif
			break;
		case '*':	/* EOF marker */
			tstr = getstring_noalloc(f, 0);
			if (strcmp(tstr, "**END OF DUMP***")) {
				fprintf(stderr,
					"\nBad EOF marker at object #%d\n",
					i);
				return -1;
			} else {
#ifdef STANDALONE
				fprintf(stderr, "\n");
				fflush(stderr);
#endif
				/* Fix up bizarro foreign DBs */

#ifdef STANDALONE
				unscraw_foreign(g_format, g_version, g_flags);
#endif
				*db_version = g_version;
				*db_format = g_format;
				*db_flags = g_flags;
#ifndef STANDALONE
				load_player_names();
#endif
#ifdef STANDALONE				
				if (!has_typed_quotas)
					fix_typed_quotas();
				if (g_format == F_MUX)
					fix_mux_zones();
#endif
				return mudstate.db_top;
			}
		default:
			fprintf(stderr, "\nIllegal character '%c' near object #%d\n",
				ch, i);
			return -1;
		}


	}

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
	/* write the attribute list */


#ifndef STANDALONE
	if ((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
#else
	if ((!(flags & V_GDBM))) {
#endif
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

#ifndef MEMORY_BASED
	al_store();
#endif /* MEMORY_BASED */

	switch (format) {
	case F_TINYMUSH:
		flags = version;
		break;
	default:
		fprintf(stderr, "Can only write TinyMUSH 3 format.\n");
		return -1;
	}
#ifdef STANDALONE
	fprintf(stderr, "Writing ");
	fflush(stderr);
#endif
	i = mudstate.attr_next;
	/* TinyMUSH 2 wrote '+V', MUX wrote '+X', 3.0 writes '+T'. */
	fprintf(f, "+T%d\n+S%d\n+N%d\n", flags, mudstate.db_top, i);
	fprintf(f, "-R%d\n", mudstate.record_players);
	
	/* Dump user-named attribute info */

	vp = vattr_first();
	while (vp != NULL) {
		if (!(vp->flags & AF_DELETED))
			fprintf(f, "+A%d\n\"%d:%s\"\n",
				vp->number, vp->flags, vp->name);
		vp = vattr_next(vp);
	}

	DO_WHOLE_DB(i) {
#ifndef MEMORY_BASED
		if (!(i % 25)) {
			cache_reset(0);
		}
#endif /* MEMORY_BASED */

#ifdef STANDALONE
		if (!(i % 100)) {
			fputc('.', stderr);
			fflush(stderr);
		}
#endif

		if (!(Going(i))) {
			fprintf(f, "!%d\n", i);
			db_write_object(f, i, format, flags);
		}
	}
	fputs("***END OF DUMP***\n", f);
	fflush(f);
#ifdef STANDALONE
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	return (mudstate.db_top);
}
