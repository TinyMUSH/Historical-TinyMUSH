/* funobj.c - object functions */
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
#include "misc.h"	/* required by code */

extern char *FDECL(upcasestr, (char *));
extern dbref FDECL(find_connected_ambiguous, (dbref, char *));

extern NAMETAB attraccess_nametab[];
extern NAMETAB indiv_attraccess_nametab[];

/* ---------------------------------------------------------------------------
 * nearby_or_control: Check if player is near or controls thing
 */

#define nearby_or_control(p,t) \
(Good_obj(p) && Good_obj(t) && (Controls(p,t) || nearby(p,t)))

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
			if (Exit_Visible(exit, player, key)) {
				safe_dbref(buff, bufc, exit);
				return;
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
					  Exit_Visible(exit, player, key)) {
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

	if (parse_attrib(player, fargs[0], &it, &atr, 1)) {
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
	safe_bool(buff, bufc, Controls(x, y));
}

/* ---------------------------------------------------------------------------
 * fun_sees: Can X see Y in the normal Contents list of the room. If X
 *           or Y do not exist, 0 is returned.
 */

FUNCTION(fun_sees)
{
    dbref it, thing;

    if ((it = match_thing(player, fargs[0])) == NOTHING) {
	safe_chr('0', buff, bufc);
	return;
    }

    thing = match_thing(player, fargs[1]);
    if (!Good_obj(thing)) {
	safe_chr('0', buff, bufc);
	return;
    }

    safe_bool(buff, bufc,
	      (isExit(thing) ?
	       Can_See_Exit(it, thing, Darkened(it, Location(thing))) :
	       Can_See(it, thing, Sees_Always(it, Location(thing)))));
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
	} else {
	    safe_bool(buff, bufc, nearby(obj1, obj2));
	}
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
		exec(buff, bufc, it, it, it, 0, &str, (char **)NULL, 0);
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
 * Locks.
 */

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
		if ((attr->flags & AF_IS_LOCK) || 
		    Read_attr(player, it, attr, aowner, aflags)) {
		    if (Pass_Locks(victim)) {
			safe_chr('1', buff, bufc);
		    } else {
			bool = parse_boolexp(player, tbuf, 1);
			safe_bool(buff, bufc, eval_boolexp(victim, it, it,
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
			if (Exit_Visible(thing, player, key)) {
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
 * fun_findable: can X locate Y
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_findable)
{
	dbref obj = match_thing(player, fargs[0]);
	dbref victim = match_thing(player, fargs[1]);

	if (obj == NOTHING)
		safe_str("#-1 ARG1 NOT FOUND", buff, bufc);
	else if (victim == NOTHING)
		safe_str("#-1 ARG2 NOT FOUND", buff, bufc);
	else
		safe_bool(buff, bufc, locatable(obj, victim, obj));
}

/* ---------------------------------------------------------------------------
 * fun_visible:  Can X examine Y. If X does not exist, 0 is returned.
 *               If Y, the object, does not exist, 0 is returned. If
 *               Y the object exists, but the optional attribute does
 *               not, X's ability to return Y the object is returned.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_visible)
{
	dbref it, thing, aowner;
	int aflags, atr;
	ATTR *ap;

	if ((it = match_thing(player, fargs[0])) == NOTHING) {
		safe_chr('0', buff, bufc);
		return;
	}
	if (parse_attrib(player, fargs[1], &thing, &atr, 1)) {
		if (atr == NOTHING) {
			safe_bool(buff, bufc, Examinable(it, thing));
			return;
		}
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		safe_bool(buff, bufc, See_attr_all(it, thing, ap, aowner,
						   aflags, 1));
		return;
	}
	thing = match_thing(player, fargs[1]);
	if (!Good_obj(thing)) {
		safe_chr('0', buff, bufc);
		return;
	}
	safe_bool(buff, bufc, Examinable(it, thing));
}

/* ------------------------------------------------------------------------
 * fun_writable: Returns 1 if player could set <obj>/<attr>.
 */

FUNCTION(fun_writable)
{
	dbref it, thing, aowner;
	int aflags, atr;
	ATTR *ap;

	if ((it = match_thing(player, fargs[0])) == NOTHING) {
		safe_chr('0', buff, bufc);
		return;
	}
	if (parse_attrib(player, fargs[1], &thing, &atr, 1) &&
	    (atr != NOTHING)) {
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		safe_bool(buff, bufc, Set_attr(it, thing, ap, aflags));
		return;
	}
	safe_chr('0', buff, bufc);
}

/* ------------------------------------------------------------------------
 * fun_flags: Returns the flags on an object.
 * Because @switch is case-insensitive, not quite as useful as it could be.
 */

FUNCTION(fun_flags)
{
	dbref it, aowner;
	int atr, aflags;
	char *buff2, xbuf[16], *xbufp;

	if (parse_attrib(player, fargs[0], &it, &atr, 1)) {
	    if (atr == NOTHING) {
		safe_nothing(buff, bufc);
	    } else {
		atr_pget_info(it, atr, &aowner, &aflags);
		Print_Attr_Flags(aflags, xbuf, xbufp);
		safe_str(xbuf, buff, bufc);
	    }
	} else {
	    it = match_thing(player, fargs[0]);
	    if ((it != NOTHING) &&
		(mudconf.pub_flags || Examinable(player, it) ||
		 (it == cause))) {
		buff2 = unparse_flags(player, it);
		safe_str(buff2, buff, bufc);
		free_sbuf(buff2);
	    } else
		safe_nothing(buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * andflags, orflags: Check a list of flags.
 */

FUNCTION(handle_flaglists)
{
	char *s;
	char flagletter[2];
	FLAGSET fset;
	FLAG p_type;
	int negate, temp, type;
	dbref it = match_thing(player, fargs[0]);

	type = ((FUN *)fargs[-1])->flags & LOGIC_OR;

	negate = temp = 0;

	if ((it == NOTHING) ||
	    (!(mudconf.pub_flags || Examinable(player, it) || (it == cause)))) {
		safe_chr('0', buff, bufc);
		return;
	}
		
	for (s = fargs[1]; *s; s++) {

		/* Check for a negation sign. If we find it, we note it and 
		 * increment the pointer to the next character. 
		 */

		if (*s == '!') {
			negate = 1;
			s++;
		} else {
			negate = 0;
		}

		if (!*s) {
			safe_chr('0', buff, bufc);
			return;
		}
		flagletter[0] = *s;
		flagletter[1] = '\0';

		if (!convert_flags(player, flagletter, &fset, &p_type)) {

			/* Either we got a '!' that wasn't followed by a
			 * letter, or we couldn't find that flag. For
			 * AND, since we've failed a check, we can
			 * return false. Otherwise we just go on. 
			 */

			if (!type) {
				safe_chr('0', buff, bufc);
				return;
			} else
				continue;

		} else {

			/* does the object have this flag? */

			if ((Flags(it) & fset.word1) ||
			    (Flags2(it) & fset.word2) ||
			    (Flags3(it) & fset.word3) ||
			    (Typeof(it) == p_type)) {
				if (isPlayer(it) && (fset.word2 == CONNECTED)
				    && Hidden(it) && !See_Hidden(player))
					temp = 0;
				else
					temp = 1;
			} else {
				temp = 0;
			}
			
			if ((!type) && ((negate && temp) || (!negate && !temp))) {

				/* Too bad there's no NXOR function... At
				 * this point we've either got a flag
				 * and we don't want it, or we don't
				 * have a flag and we want it. Since
				 * it's AND, we return false. 
				 */
				safe_chr('0', buff, bufc);
				return;

			} else if ((type) &&
				 ((!negate && temp) || (negate && !temp))) {

				/* We've found something we want, in an OR. */

				safe_chr('1', buff, bufc);
				return;
			}
			/* Otherwise, we don't need to do anything. */
		}
	}
	safe_bool(buff, bufc, !type);
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
    int flagval;

    if (!See_attr(player, thing, attr, aowner, aflags))
	return 0;

    flagval = search_nametab(player, indiv_attraccess_nametab, flagname);
    if (flagval < 0)
	flagval = search_nametab(player, attraccess_nametab, flagname);
    if (flagval < 0)
	return 0;

    return (aflags & flagval);
}

FUNCTION(fun_hasflag)
{
    dbref it, aowner;
    int atr, aflags;
    ATTR *ap;

    if (parse_attrib(player, fargs[0], &it, &atr, 1)) {
	if (atr == NOTHING) {
	    safe_str("#-1 NOT FOUND", buff, bufc);
	} else {
	    ap = atr_num(atr);
	    atr_pget_info(it, atr, &aowner, &aflags);
	    safe_bool(buff, bufc, 
		      atr_has_flag(player, it, ap, aowner, aflags, fargs[1]));
	}
    } else {
	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
	    safe_str("#-1 NOT FOUND", buff, bufc);
	    return;
	}
	if (mudconf.pub_flags || Examinable(player, it) || (it == cause)) {
	    safe_bool(buff, bufc, has_flag(player, it, fargs[1]));
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
		safe_bool(buff, bufc, has_power(player, it, fargs[1]));
	} else {
		safe_noperm(buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * Timestamps.
 */

FUNCTION(fun_lastaccess)
{
    dbref it = match_thing(player, fargs[0]);

    if (!Good_obj(it) || !Examinable(player, it)) {
	safe_known_str("-1", 2, buff, bufc);
    } else {
	safe_ltos(buff, bufc, AccessTime(it));
    }
}

FUNCTION(fun_lastmod)
{
    dbref it = match_thing(player, fargs[0]);

    if (!Good_obj(it) || !Examinable(player, it)) {
	safe_known_str("-1", 2, buff, bufc);
    } else {
	safe_ltos(buff, bufc, ModTime(it));
    }
}

/* ---------------------------------------------------------------------------
 * Parent-child relationships.
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

FUNCTION(fun_lparent)
{
	dbref it;
	dbref par;
	char tbuf1[20];
	int i;

	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!(Examinable(player, it))) {
		safe_noperm(buff, bufc);
		return;
	}
	sprintf(tbuf1, "#%d", it);
	safe_str(tbuf1, buff, bufc);
	par = Parent(it);

	i = 1;
	while (Good_obj(par) && Examinable(player, it) &&
	    (i < mudconf.parent_nest_lim)) {
	    sprintf(tbuf1, " #%d", par);
	    safe_str(tbuf1, buff, bufc);
	    it = par;
	    par = Parent(par);
	    i++;
	}
}

FUNCTION(fun_children)
{
	dbref i, it = match_thing(player, fargs[0]);
	char *bb_p;

	if (!Controls(player, it) && !See_All(player)) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}
	bb_p = *bufc;
	DO_WHOLE_DB(i) {
	    if (Parent(i) == it) {
		if (*bufc != bb_p) {
		    safe_chr(' ', buff, bufc);
		}
		safe_dbref(buff, bufc, i);
	    }
	}
}

/* ---------------------------------------------------------------------------
 * Zones.
 */

FUNCTION(fun_zone)
{
	dbref it;

	if (!mudconf.have_zones) {
		return;
	}
	it = match_thing(player, fargs[0]);
	if (it == NOTHING || !Examinable(player, it)) {
		safe_nothing(buff, bufc);
		return;
	}
	safe_dbref(buff, bufc, Zone(it));
}

FUNCTION(scan_zone)
{
	dbref i, it = match_thing(player, fargs[0]);
	int type;
	char *bb_p;

	type = ((FUN *)fargs[-1])->flags & TYPE_MASK;
	
	if (!mudconf.have_zones || (!Controls(player, it) && !WizRoy(player))) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}
	bb_p = *bufc;
	DO_WHOLE_DB(i) {
		if (Typeof(i) == type) {
			if (Zone(i) == it) {
				if (*bufc != bb_p) {
					safe_chr(' ', buff, bufc);
				}
				safe_dbref(buff, bufc, i);
			}
		}
	}
}

FUNCTION(fun_zfun)
{
	dbref aowner;
	int aflags, alen;
	ATTR *ap;
	char *tbuf1, *str;

	dbref zone = Zone(player);

	if (!mudconf.have_zones) {
		safe_str("#-1 ZONES DISABLED", buff, bufc);
		return;
	}
	if (zone == NOTHING) {
		safe_str("#-1 INVALID ZONE", buff, bufc);
		return;
	}
	if (!fargs[0] || !*fargs[0])
		return;

	/* find the user function attribute */
	ap = atr_str(upcasestr(fargs[0]));
	if (!ap) {
		safe_str("#-1 NO SUCH USER FUNCTION", buff, bufc);
		return;
	}
	tbuf1 = atr_pget(zone, ap->number, &aowner, &aflags, &alen);
	if (!See_attr(player, zone, ap, aowner, aflags)) {
		safe_str("#-1 NO PERMISSION TO GET ATTRIBUTE", buff, bufc);
		free_lbuf(tbuf1);
		return;
	}
	str = tbuf1;
	/* Behavior here is a little wacky. The enactor was always the player,
	 * not the cause. You can still get the caller, though.
	 */
	exec(buff, bufc, zone, caller, player,
	     EV_EVAL | EV_STRIP | EV_FCHECK, &str, &(fargs[1]), nfargs - 1);
	free_lbuf(tbuf1);
}

/* ---------------------------------------------------------------------------
 * fun_hasattr: does object X have attribute Y.
 */

FUNCTION(fun_hasattr)
{
	dbref thing, aowner;
	int aflags, alen;
	ATTR *attr;
	char *tbuf;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING) {
		safe_nomatch(buff, bufc);
   		return;
	} else if (!Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	attr = atr_str(fargs[1]);
	if (!attr) {
		safe_chr('0', buff, bufc);
		return;
	}
	atr_get_info(thing, attr->number, &aowner, &aflags);
	if (!See_attr(player, thing, attr, aowner, aflags)) {
		safe_chr('0', buff, bufc);
	} else {
		tbuf = atr_get(thing, attr->number, &aowner, &aflags, &alen);
		if (*tbuf) {
			safe_chr('1', buff, bufc);
		} else {
			safe_chr('0', buff, bufc);
		}
		free_lbuf(tbuf);
	}
}

FUNCTION(fun_hasattrp)
{
	dbref thing, aowner;
	int aflags, alen;
	ATTR *attr;
	char *tbuf;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	attr = atr_str(fargs[1]);
	if (!attr) {
		safe_chr('0', buff, bufc);
		return;
	}
	atr_pget_info(thing, attr->number, &aowner, &aflags);
	if (!See_attr(player, thing, attr, aowner, aflags)) {
		safe_chr('0', buff, bufc);
	} else {
		tbuf = atr_pget(thing, attr->number, &aowner, &aflags, &alen);
		if (*tbuf) {
			safe_chr('1', buff, bufc);
		} else {
			safe_chr('0', buff, bufc);
		}
		free_lbuf(tbuf);
	}
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
	exec(buff, bufc, player, caller, cause, EV_FIGNORE, &str,
	     cargs, ncargs);
	free_sbuf(sbuf);
}

/* ---------------------------------------------------------------------------
 * fun_get, fun_get_eval: Get attribute from object.
 */

static void perform_get(player, str, buff, bufc)
    dbref player;
    char *str;
    char *buff, **bufc;
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, alen, rval;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    if ((rval = parse_attrib(player, str, &thing, &attrib, 0)) == 0) {
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

    if (free_buffer)
	safe_known_str(atr_gotten, alen, buff, bufc);
    else
	safe_str(atr_gotten, buff, bufc);

    if (free_buffer)
	free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_get)
{
	perform_get(player, fargs[0], buff, bufc);
}

FUNCTION(fun_xget)
{
	if (!*fargs[0] || !*fargs[1])
		return;
	perform_get(player, tprintf("%s/%s", fargs[0], fargs[1]),
		    buff, bufc);
}

static void perform_get_eval(player, str, buff, bufc)
    dbref player;
    char *str;
    char *buff, **bufc;
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, alen, eval_it;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

	if (!parse_attrib(player, str, &thing, &attrib, 0)) {
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
	if (eval_it) {
		str = atr_gotten;
		exec(buff, bufc, thing, player, player,
		     EV_FIGNORE | EV_EVAL, &str, (char **)NULL, 0);
	} else {
		safe_str(atr_gotten, buff, bufc);
	}
	if (free_buffer)
		free_lbuf(atr_gotten);
	return;
}

FUNCTION(fun_get_eval)
{
    perform_get_eval(player, fargs[0], buff, bufc);
}


FUNCTION(fun_eval)
{
    char *str;

    if ((nfargs != 1) && (nfargs != 2)) {
	safe_str("#-1 FUNCTION (EVAL) EXPECTS 1 OR 2 ARGUMENTS", buff, bufc);
	return;
    }
    if (nfargs == 1) {
	str = fargs[0];
	exec(buff, bufc, player, caller, cause, EV_EVAL|EV_FCHECK,
	     &str, (char **)NULL, 0);
	return;
    }
    if (!*fargs[0] || !*fargs[1])
	return;
    perform_get_eval(player, tprintf("%s/%s", fargs[0], fargs[1]), buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_u and fun_ulocal:  Call a user-defined function.
 */

static void do_ufun(buff, bufc, player, caller, cause,
		    fargs, nfargs,
		    cargs, ncargs,
		    is_local)
char *buff, **bufc;
dbref player, caller, cause;
char *fargs[], *cargs[];
int nfargs, ncargs, is_local;
{
	dbref aowner, thing;
	int aflags, alen, anum, preserve_len[MAX_GLOBAL_REGS];
	ATTR *ap;
	char *atext, *preserve[MAX_GLOBAL_REGS], *str;
	
	/* We need at least one argument */

	if (nfargs < 1) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
		return;
	}
	/* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

	Parse_Uattr(player, fargs[0], thing, anum, ap);
	Get_Uattr(player, thing, ap, atext, aowner, aflags, alen);

	/* If we're evaluating locally, preserve the global registers. */

	if (is_local) {
		save_global_regs("fun_ulocal_save", preserve, preserve_len);
	}
	
	/* Evaluate it using the rest of the passed function args */

	str = atext;
	exec(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL, &str,
	     &(fargs[1]), nfargs - 1);
	free_lbuf(atext);

	/* If we're evaluating locally, restore the preserved registers. */

	if (is_local) {
		restore_global_regs("fun_ulocal_restore", preserve,
				    preserve_len);
	}
}

FUNCTION(fun_u)
{
	do_ufun(buff, bufc, player, caller, cause,
		fargs, nfargs, cargs, ncargs, 0);
}

FUNCTION(fun_ulocal)
{
	do_ufun(buff, bufc, player, caller, cause,
		fargs, nfargs, cargs, ncargs, 1);
}

/* ---------------------------------------------------------------------------
 * fun_localize: Evaluate a function with local scope (i.e., preserve and
 * restore the r-registers). Essentially like calling ulocal() but with the
 * function string directly.
 */

FUNCTION(fun_localize)
{
    char *str, *preserve[MAX_GLOBAL_REGS];
    int preserve_len[MAX_GLOBAL_REGS];

    save_global_regs("fun_localize_save", preserve, preserve_len);

    str = fargs[0];
    exec(buff, bufc, player, caller, cause,
	 EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);

    restore_global_regs("fun_localize_restore", preserve, preserve_len);
}

/* ---------------------------------------------------------------------------
 * fun_default, fun_edefault, and fun_udefault:
 * These check for the presence of an attribute. If it exists, then it
 * is gotten, via the equivalent of get(), get_eval(), or u(), respectively.
 * Otherwise, the default message is used.
 * In the case of udefault(), the remaining arguments to the function
 * are used as arguments to the u().
 */

FUNCTION(fun_default)
{
	dbref thing, aowner;
	int attrib, aflags, alen;
	ATTR *attr;
	char *objname, *atr_gotten, *bp, *str;

	objname = bp = alloc_lbuf("fun_default");
	str = fargs[0];
	exec(objname, &bp, player, caller, cause,
	     EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
	*bp = '\0';

	/* First we check to see that the attribute exists on the object.
	 * If so, we grab it and use it. 
	 */

	if (objname != NULL) {
	        if (parse_attrib(player, objname, &thing, &attrib, 0) &&
		    (attrib != NOTHING)) {
			attr = atr_num(attrib);
			if (attr && !(attr->flags & AF_IS_LOCK)) {
				atr_gotten = atr_pget(thing, attrib,
						      &aowner, &aflags, &alen);
				if (*atr_gotten) {
					safe_known_str(atr_gotten, alen,
						       buff, bufc);
					free_lbuf(atr_gotten);
					free_lbuf(objname);
					return;
				}
				free_lbuf(atr_gotten);
			}
		}
		free_lbuf(objname);
	}
	/* If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default. 
	 */

	str = fargs[1];
	exec(buff, bufc, player, caller, cause,
	     EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

FUNCTION(fun_edefault)
{
	dbref thing, aowner;
	int attrib, aflags, alen;
	ATTR *attr;
	char *objname, *atr_gotten, *bp, *str;

	objname = bp = alloc_lbuf("fun_edefault");
	str = fargs[0];
	exec(objname, &bp, player, caller, cause,
	     EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
	*bp = '\0';

	/* First we check to see that the attribute exists on the object. 
	 * If so, we grab it and use it. 
	 */

	if (objname != NULL) {
		if (parse_attrib(player, objname, &thing, &attrib, 0) &&
		    (attrib != NOTHING)) {
			attr = atr_num(attrib);
			if (attr && !(attr->flags & AF_IS_LOCK)) {
				atr_gotten = atr_pget(thing, attrib,
						      &aowner, &aflags, &alen);
				if (*atr_gotten) {
					str = atr_gotten;
					exec(buff, bufc, thing, player,
					     player, EV_FIGNORE | EV_EVAL,
					     &str, (char **)NULL, 0);
					free_lbuf(atr_gotten);
					free_lbuf(objname);
					return;
				}
				free_lbuf(atr_gotten);
			}
		}
		free_lbuf(objname);
	}
	/* If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default. 
	 */

	str = fargs[1];
	exec(buff, bufc, player, caller, cause,
	     EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

FUNCTION(fun_udefault)
{
    dbref thing, aowner;
    int aflags, alen, anum, i, j;
    ATTR *ap;
    char *objname, *atext, *str, *bp, *xargs[NUM_ENV_VARS];

    if (nfargs < 2)		/* must have at least two arguments */
	return;

    str = fargs[0];
    objname = bp = alloc_lbuf("fun_udefault");
    exec(objname, &bp, player, caller, cause,
	 EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
    *bp = '\0';

    /* First we check to see that the attribute exists on the object.
     * If so, we grab it and use it. 
     */

    if (objname != NULL) {
	Parse_Uattr(player, objname, thing, anum, ap);
	if (ap) {
	    atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	    if (*atext) {
		/* Now we have a problem -- we've got to go eval
		 * all of those arguments to the function. 
		 */
		for (i = 2, j = 0; j < NUM_ENV_VARS; i++, j++) {
		    if ((i < nfargs) && fargs[i]) {
			bp = xargs[j] = alloc_lbuf("fun_udefault_args");
			str = fargs[i];
			exec(xargs[j], &bp, player, caller, cause,
			     EV_STRIP | EV_FCHECK | EV_EVAL,
			     &str, cargs, ncargs);
		    } else {
			xargs[j] = NULL;
		    } 
		}
    
		str = atext;
		exec(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL,
		     &str, xargs, nfargs - 2);

		/* Then clean up after ourselves. */

		for (j = 0; j < NUM_ENV_VARS; j++)
		    if (xargs[j])
			free_lbuf(xargs[j]);

		free_lbuf(atext);
		free_lbuf(objname);
		return;
	    }
	    free_lbuf(atext);
	}
	free_lbuf(objname);

    }
    /* If we've hit this point, we've not gotten anything useful, so we 
     * go and evaluate the default. 
     */

    str = fargs[1];
    exec(buff, bufc, player, caller, cause,
	 EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/*---------------------------------------------------------------------------
 * Evaluate from a specific object's perspective.
 */

FUNCTION(fun_objeval)
{
	dbref obj;
	char *name, *bp, *str;

	if (!*fargs[0]) {
		return;
	}
	name = bp = alloc_lbuf("fun_objeval");
	str = fargs[0];
	exec(name, &bp, player, caller, cause,
	     EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	*bp = '\0';
	obj = match_thing(player, name);

	/* In order to evaluate from something else's viewpoint, you must
	 * have the same owner as it, or be a wizard (unless
	 * objeval_requires_control is turned on, in which case you
	 * must control it, period). Otherwise, we default to evaluating
	 * from our own viewpoint. Also, you cannot evaluate things from
	 * the point of view of God.
	 */
	if ((obj == NOTHING) || God(obj) ||
	    (mudconf.fascist_objeval ?
	     !Controls(player, obj) :
	     ((Owner(obj) != Owner(player)) && !Wizard(player)))) {
		obj = player;
	}

	str = fargs[1];
	exec(buff, bufc, obj, player, cause,
	     EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	free_lbuf(name);
}

/* ---------------------------------------------------------------------------
 * Matching functions.
 */

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
	*tp = tolower(*tp);
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
 * fun_lattr: Return list of attributes I can see on the object.
 * fun_nattr: Ditto, but just count 'em up.
 */

FUNCTION(handle_lattr)
{
	dbref thing;
	ATTR *attr;
	char *bb_p;
	int ca, total = 0, count_only;

	count_only = ((FUN *)fargs[-1])->flags & LATTR_COUNT;

	/* Check for wildcard matching.  parse_attrib_wild checks for read
	 * permission, so we don't have to.  Have p_a_w assume the
	 * slash-star if it is missing. 
	 */

	olist_push();
	if (parse_attrib_wild(player, fargs[0], &thing, 0, 0, 1, 1)) {
		bb_p = *bufc;
		for (ca = olist_first(); ca != NOTHING; ca = olist_next()) {
			attr = atr_num(ca);
			if (attr) {
			    if (count_only) {
				total++;
			    } else {
				if (*bufc != bb_p) {
					safe_chr(' ', buff, bufc);
				}
				safe_str((char *)attr->name, buff, bufc);
			    }
			}
		}
		if (count_only)
			safe_ltos(buff, bufc, total);
	} else {
		if (!mudconf.lattr_oldstyle) {
			safe_nomatch(buff, bufc);
		} else if (count_only) {
			safe_chr('0', buff, bufc);
		}
	}
	olist_pop();
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
	safe_tprintf_str(buff, bufc, "%d %d %d %d %d %d %d %d",
			 statinfo.s_total, statinfo.s_rooms,
			 statinfo.s_exits, statinfo.s_things,
			 statinfo.s_players, statinfo.s_unknown,
			 statinfo.s_going, statinfo.s_garbage);
}

/* ---------------------------------------------------------------------------
 * Memory usage.
 */

static int mem_usage(thing)
dbref thing;
{
	int k;
	int ca;
	char *as, *str;
	ATTR *attr;

	k = sizeof(OBJ);

	k += strlen(Name(thing)) + 1;
	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as)) {
		str = atr_get_raw(thing, ca);
		if (str && *str)
			k += strlen(str);
		attr = atr_num(ca);
		if (attr) {
			str = (char *)attr->name;
			if (str && *str)
				k += strlen(((ATTR *) atr_num(ca))->name);
		}
	}
	return k;
}

static int mem_usage_attr(player, str)
    dbref player;
    char *str;
{
    dbref thing, aowner;
    int atr, aflags, alen;
    char *abuf;
    ATTR *ap;
    int bytes_atext = 0;

    olist_push();
    if (parse_attrib_wild(player, str, &thing, 0, 0, 1, 1)) {
	for (atr = olist_first(); atr != NOTHING; atr = olist_next()) {
	    ap = atr_num(atr);
	    if (!ap)
		continue;
	    abuf = atr_get(thing, atr, &aowner, &aflags, &alen);
	    /* Player must be able to read attribute with 'examine' */
	    if (Examinable(player, thing) &&
		Read_attr(player, thing, ap, aowner, aflags))
		bytes_atext += strlen(abuf);
	    free_lbuf(abuf);
	}
    }

    olist_pop();
    return bytes_atext;
}

FUNCTION(fun_objmem)
{
	dbref thing;

	if (strchr(fargs[0], '/')) {
	    safe_ltos(buff, bufc, mem_usage_attr(player, fargs[0]));
	    return;
	}

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING || !Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	safe_ltos(buff, bufc, mem_usage(thing));
}

FUNCTION(fun_playmem)
{
	int tot = 0;
	dbref thing;
	dbref j;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING || !Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	DO_WHOLE_DB(j)
		if (Owner(j) == thing)
		tot += mem_usage(j);
	safe_ltos(buff, bufc, tot);
}

/* ---------------------------------------------------------------------------
 * Type functions.
 */

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

FUNCTION(fun_hastype)
{
	dbref it = match_thing(player, fargs[0]);

	if (it == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (!fargs[1] || !*fargs[1]) {
		safe_str("#-1 NO SUCH TYPE", buff, bufc);
		return;
	}
	switch (*fargs[1]) {
	case 'r':
	case 'R':
		safe_chr((Typeof(it) == TYPE_ROOM) ? '1' : '0', buff, bufc);
		break;
	case 'e':
	case 'E':
		safe_chr((Typeof(it) == TYPE_EXIT) ? '1' : '0', buff, bufc);
		break;
	case 'p':
	case 'P':
		safe_chr((Typeof(it) == TYPE_PLAYER) ? '1' : '0', buff, bufc);
		break;
	case 't':
	case 'T':
		safe_chr((Typeof(it) == TYPE_THING) ? '1' : '0', buff, bufc);
		break;
	default:
		safe_str("#-1 NO SUCH TYPE", buff, bufc);
		break;
	};
}

/* ---------------------------------------------------------------------------
 * fun_lastcreate: Return the last object of type Y that X created.
 */

FUNCTION(fun_lastcreate)
{
    int i, aowner, aflags, alen, obj_list[4], obj_type;
    char *obj_str, *p, *tokst;
    dbref obj = match_thing(player, fargs[0]);

    if (!controls(player, obj)) {    /* Automatically checks for GoodObj */
	safe_nothing(buff, bufc);
	return;
    }

    switch (*fargs[1]) {
      case 'R':
      case 'r':
	obj_type = 0;
	break;
      case 'E':
      case 'e':
	obj_type = 1;;
	break;
      case 'T':
      case 't':
	obj_type = 2;
	break;
      case 'P':
      case 'p':
	obj_type = 3;
	break;
      default:
	notify_quiet(player, "Invalid object type.");
	safe_nothing(buff, bufc);
	return;
    }

    obj_str = atr_get(obj, A_NEWOBJS, &aowner, &aflags, &alen);
    if (!*obj_str) {
	free_lbuf(obj_str);
	safe_nothing(buff, bufc);
	return;
    }

    for (p = strtok_r(obj_str, " ", &tokst), i = 0;
	 p && (i < 4);
	 p = strtok_r(NULL, " ", &tokst), i++) {
	obj_list[i] = atoi(p);
    }
    free_lbuf(obj_str);

    safe_dbref(buff, bufc, obj_list[obj_type]);
}
