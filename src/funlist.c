/* funlist.c - list functions */
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
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "ansi.h"	/* required by code */

/* ---------------------------------------------------------------------------
 * List management utilities.
 */

#define	ALPHANUM_LIST	1
#define	NUMERIC_LIST	2
#define	DBREF_LIST	3
#define	FLOAT_LIST	4
#define NOCASE_LIST	5

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
			} else if (strchr(ptrs[i], '.')) {
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
		switch (tolower(*fargs[type_pos - 1])) {
		case 'd':
			return DBREF_LIST;
		case 'n':
			return NUMERIC_LIST;
		case 'f':
			return FLOAT_LIST;
		case 'i':
			return NOCASE_LIST;
		case '\0':
			return autodetect_list(ptrs, nitems);
		default:
			return ALPHANUM_LIST;
		}
	}
	return autodetect_list(ptrs, nitems);
}

static int dbnum(dbr)
char *dbr;
{
	if ((*dbr != '#') || (dbr[1] == '\0'))
		return 0;
	else
		return atoi(dbr + 1);
}

/* ---------------------------------------------------------------------------
 * fun_words: Returns number of words in a string. Aka vdim.
 * Added 1/28/91 Philip D. Wasson
 */

FUNCTION(fun_words)
{
	Delim isep;

	if (nfargs == 0) {
		safe_chr('0', buff, bufc);
		return;
	}
	VaChk_Only_In(2);
	safe_ltos(buff, bufc, countwords(fargs[0], &isep));
}

/* ---------------------------------------------------------------------------
 * fun_first: Returns first word in a string
 */

FUNCTION(fun_first)
{
	char *s, *first;
	Delim isep;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In(2);
	s = trim_space_sep(fargs[0], &isep);	/* leading spaces */
	first = split_token(&s, &isep);
	if (first) {
		safe_str(first, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_rest: Returns all but the first word in a string 
 */

FUNCTION(fun_rest)
{
	char *s, *rest;
	Delim isep; 
	int ansi_state = ANST_NONE;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In(2);
	s = trim_space_sep(fargs[0], &isep);	/* leading spaces */
	rest = next_token_ansi(s, &isep, &ansi_state);
	if (rest) {
		safe_str(ansi_transition_esccode(ANST_NORMAL, ansi_state), buff, bufc);
		safe_str(rest, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_last: Returns last word in a string
 */

FUNCTION(fun_last)
{
	char *s, *last;
	Delim isep;
	int ansi_state = ANST_NONE;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In(2);

	if (isep.len == 1) {

		last = s = trim_space_sep(fargs[0], &isep);

		do {
			/* this is like next_token(), but tracking ansi */
			while (*s == ESC_CHAR) {
				track_esccode(s, ansi_state);
			}
			while (*s && (*s != isep.str[0])) {
				++s;
				while (*s == ESC_CHAR) {
					track_esccode(s, ansi_state);
				}
			}
			if (*s) {
				++s;
				if (isep.str[0] == ' ') {
					while (*s == ' ')
						++s;
				}
				last = s;
			}
		} while (*s);

		safe_str(ansi_transition_esccode(ANST_NORMAL, ansi_state), buff, bufc);
		safe_known_str(last, s - last, buff, bufc);

	} else {

	    s = fargs[0];

	    /* Walk backwards through the string to find the separator.
	     * Find the last character, and compare the previous characters,
	     * to find the separator. If we can't find the last character
	     * or we know we're going to fall off the string, return
	     * the original string.
	     */

	    if ((last = strrchr(s, isep.str[isep.len - 1])) == NULL) {
		safe_str(s, buff, bufc);
		return;
	    }
	    while (last >= s + isep.len - 1) {
		if ((*last == isep.str[isep.len - 1]) &&
		    !strncmp(isep.str, last - isep.len + 1, isep.len)) {
		    safe_str(++last, buff, bufc);
		    return;
		}
		last--;
	    }
	    safe_str(s, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_match: Match arg2 against each word of arg1, returning index of
 * first match.
 */

FUNCTION(fun_match)
{
	int wcount;
	char *r, *s;
	Delim isep;

	VaChk_Only_In(3);

	/* Check each word individually, returning the word number of the 
	 * first one that matches.  If none match, return 0. 
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);
	do {
		r = split_token(&s, &isep);
		if (quick_wild(fargs[1], r)) {
			safe_ltos(buff, bufc, wcount);
			return;
		}
		wcount++;
	} while (s);
	safe_chr('0', buff, bufc);
}

FUNCTION(fun_matchall)
{
	int wcount;
	char *r, *s, *old;
	Delim isep, osep;

	VaChk_Only_In_Out(4);

	/* SPECIAL CASE: If there's no output delimiter specified, we use
	 * a space, NOT the delimiter given for the list!
	 */
	if (nfargs < 4) {
	    osep.str[0] = ' ';
	    osep.len = 1;
	}

	old = *bufc;

	/* Check each word individually, returning the word number of all 
	 * that match. If none match, return a null string.
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);
	do {
		r = split_token(&s, &isep);
		if (quick_wild(fargs[1], r)) {
			if (old != *bufc) {
			    print_sep(&osep, buff, bufc);
			}
			safe_ltos(buff, bufc, wcount);
		}
		wcount++;
	} while (s);
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
	char *r, *s, *t;
	Delim isep, osep;

	VaChk_Only_In_Out(5);

	s = fargs[0];
	start = atoi(fargs[1]);
	len = atoi(fargs[2]);

	if ((start < 1) || (len < 1)) {
		return;
	}
	/* Skip to the start of the string to save */

	start--;
	s = trim_space_sep(s, &isep);
	while (start && s) {
		s = next_token(s, &isep);
		start--;
	}

	/* If we ran of the end of the string, return nothing */

	if (!s || !*s) {
		return;
	}

	/* If our delimiter is the same, we have an easy task. Otherwise
	 * we have to go token by token.
	 */

	if (!strcmp((&isep)->str, (&osep)->str)) {
	     /* Count off the words in the string to save */
	     r = s;
	     len--;
	     while (len && s) {
		s = next_token(s, &isep);
		len--;
	     }
	     /* Chop off the rest of the string, if needed */
	     if (s && *s)
		  t = split_token(&s, &isep);
	     safe_str(r, buff, bufc);
	} else {
	     r = *bufc;
	     do {
		  t = split_token(&s, &isep);
		  if (r != *bufc) {
		       print_sep(&osep, buff, bufc);
		  }
		  safe_str(t, buff, bufc);
		  len--;
	     } while (len && s && *s);
	}
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
		if ((s = strchr(s, c)) != NULL)
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
		if ((p = strchr(p, c)) != NULL) {
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

/* ---------------------------------------------------------------------------
 * ldelete: Remove a word from a string by place
 *  ldelete(<list>,<position>[,<separator>])
 *
 * insert: insert a word into a string by place
 *  insert(<list>,<position>,<new item> [,<separator>])
 *
 * replace: replace a word into a string by place
 *  replace(<list>,<position>,<new item>[,<separator>])
 *
 * lreplace: replace multiple words into a string by places
 *  lreplace(<list>,<replacement words>,<positions>[,<isep>,<osep>])
 */

#define	IF_DELETE	0
#define	IF_REPLACE	1
#define	IF_INSERT	2

static void do_itemfuns(buff, bufc, str, el, word, sep, flag)
char *buff, **bufc, *str, *word;
const Delim *sep;
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
		for (ct = el; ct > 2 && eptr;
		     eptr = next_token(eptr, sep), ct--) ;
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
			if (eptr) {
			    print_sep(sep, buff, bufc);
			}
		}
		if (eptr) {
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_REPLACE:	/* replacing */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			print_sep(sep, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (eptr) {
			print_sep(sep, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_INSERT:	/* insertion */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			print_sep(sep, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (iptr) {
			print_sep(sep, buff, bufc);
			safe_str(iptr, buff, bufc);
		}
		if (eptr) {
			print_sep(sep, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	}
}

FUNCTION(fun_ldelete)
{				/* delete a word at position X of a list */
	Delim isep;

	VaChk_Only_In(3);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), NULL,
		    &isep, IF_DELETE);
}

FUNCTION(fun_replace)
{				/* replace a word at position X of a list */
	Delim isep;

	VaChk_Only_In(4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2],
		    &isep, IF_REPLACE);
}

FUNCTION(fun_insert)
{				/* insert a word at position X of a list */
	Delim isep;

	VaChk_Only_In(4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2],
		    &isep, IF_INSERT);
}

FUNCTION(fun_lreplace)
{
     Delim isep;
     Delim osep;
     char *origlist, *replist, *poslist;
     char **orig_p, **rep_p, **pos_p;
     int norig, nrep, npos, i, cpos;

     /* We're generous with the argument checking, in case the
      * replacement list is blank, and/or the position list is blank.
      */

     VaChk_In_Out(1, 5);

     /* If there are no positions to replace, then we just return the
      * original list.
      */

     if ((nfargs < 3) || !fargs[2]) {
	  safe_str(fargs[0], buff, bufc);
	  return;
     }

     /* The number of elements we have in our replacement list must equal
      * the number of elements in our position list.
      */

     if (!fargs[1] || 
	 (countwords(fargs[1], &isep) != countwords(fargs[2], &SPACE_DELIM))) {
	  safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
	  return;
     }

     /* Turn out lists into arrays for ease of manipulation. */

     origlist = alloc_lbuf("fun_lreplace.orig");
     replist = alloc_lbuf("fun_lreplace.rep");
     poslist = alloc_lbuf("fun_lreplace.pos");
     strcpy(origlist, fargs[0]);
     strcpy(replist, fargs[1]);
     strcpy(poslist, fargs[2]);

     norig = list2arr(&orig_p, LBUF_SIZE / 2, origlist, &isep);
     nrep = list2arr(&rep_p, LBUF_SIZE / 2, replist, &isep);
     npos = list2arr(&pos_p, LBUF_SIZE / 2, poslist, &SPACE_DELIM);

     /* The positions we have aren't necessarily sequential, so we can't
      * just walk through the list. We have to replace position by position.
      * If we get an invalid position number, just ignore it.
      */

     for (i = 0; i < npos; i++) {
	  cpos = atoi(pos_p[i]);
	  if ((cpos > 0) && (cpos <= norig))
	       orig_p[cpos - 1] = rep_p[i];
     }

     arr2list(orig_p, norig, buff, bufc, &osep);
     XFREE(orig_p, "fun_lreplace.orig_p");
     XFREE(rep_p, "fun_lreplace.rep_p");
     XFREE(pos_p, "fun_lreplace.pos_p");
     free_lbuf(origlist);
     free_lbuf(replist);
     free_lbuf(poslist);
}

/* ---------------------------------------------------------------------------
 * fun_remove: Remove a word from a string
 */

FUNCTION(fun_remove)
{
	char *s, *sp, *word, *bb_p;
	Delim isep;
	int found;

	VaChk_Only_In(3);
	if (((isep.len == 1) && strchr(fargs[1], isep.str[0])) ||
	    ((isep.len > 1) && strstr(fargs[1], isep.str))) {
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
	bb_p = *bufc;
	while (s) {
		sp = split_token(&s, &isep);
		if (found || strcmp(sp, word)) {
			if (*bufc != bb_p) {
				print_sep(&isep, buff, bufc);
			}
			safe_str(sp, buff, bufc);
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
	char *r, *s;
	Delim isep;

	VaChk_Only_In(3);
	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);
	do {
		r = split_token(&s, &isep);
		if (!strcmp(fargs[1], r)) {
			safe_ltos(buff, bufc, wcount);
			return;
		}
		wcount++;
	} while (s);
	safe_chr('0', buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_revwords: Reverse the order of words in a list.
 */

FUNCTION(fun_revwords)
{
	char *bb_p, **elems;
	Delim isep;
	int n_elems, i;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In(2);

	/* Nasty bounds checking */

	if ((int)strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff) - 1) {
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff) - 1)) = '\0';
	}

	/* Chop it up into an array of words and reverse them. */

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	bb_p = *bufc;
	for (i = n_elems - 1; i >= 0; i--) {
	    if (*bufc != bb_p) {
		print_sep(&isep, buff, bufc);
	    }
	    safe_str(elems[i], buff, bufc);
	}
	XFREE(elems, "fun_revwords.elems");
}

/* ---------------------------------------------------------------------------
 * fun_splice: given two lists and a word, merge the two lists
 *   by replacing words in list1 that are the same as the given 
 *   word by the corresponding word in list2 (by position).
 *   The lists must have the same number of words. Compare to MERGE().
 */

FUNCTION(fun_splice)
{
	char *p1, *p2, *q1, *q2, *bb_p;
	Delim isep, osep;
	int words, i;

	VaChk_Only_In_Out(5);

	/* length checks */

	if (countwords(fargs[2], &isep) > 1) {
		safe_str("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}
	words = countwords(fargs[0], &isep);
	if (words != countwords(fargs[1], &isep)) {
		safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}
	/* loop through the two lists */

	p1 = fargs[0];
	q1 = fargs[1];
	bb_p = *bufc;
	for (i = 0; i < words; i++) {
		p2 = split_token(&p1, &isep);
		q2 = split_token(&q1, &isep);
		if (*bufc != bb_p) {
		    print_sep(&osep, buff, bufc);
		}
		if (!strcmp(p2, fargs[2]))
			safe_str(q2, buff, bufc);	/* replace */
		else
			safe_str(p2, buff, bufc);	/* copy */
	}
}

/* ---------------------------------------------------------------------------
 * handle_sort: Sort lists.
 */

typedef struct f_record f_rec;
typedef struct i_record i_rec;
typedef struct a_record a_rec;

struct f_record {
	double data;
	char *str;
	int pos;
};

struct i_record {
	long data;
	char *str;
	int pos;
};

struct a_record {
	char *str;
	int pos;
};

static int a_comp(s1, s2)
const void *s1, *s2;
{
	return strcmp(*(char **)s1, *(char **)s2);
}

static int c_comp(s1, s2)
const void *s1, *s2;
{
	return strcasecmp(*(char **)s1, *(char **)s2);
}

static int arec_comp(s1, s2)
const void *s1, *s2;
{
	return strcmp(((a_rec *) s1)->str, ((a_rec *) s2)->str);
}

static int crec_comp(s1, s2)
const void *s1, *s2;
{
	return strcasecmp(((a_rec *) s1)->str, ((a_rec *) s2)->str);
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

#define Get_Poslist(p, n, l) \
    l = (int *) XCALLOC(n, sizeof(int), "do_asort.poslist"); \
    for (i = 0; i < n; i++) l[i] = p[i].pos;

static int *do_asort(s, n, sort_type, listpos_only)
char *s[];
int n, sort_type;
{
	int i;
	f_rec *fp = NULL;
	i_rec *ip = NULL;
	a_rec *ap = NULL;
	int *poslist = NULL;

	switch (sort_type) {
	case ALPHANUM_LIST:
		if (!listpos_only) {
			qsort((void *)s, n, sizeof(char *), 
			      (int (*)(const void *, const void *))a_comp);
		} else {
			ap = (a_rec *) XCALLOC(n, sizeof(a_rec), "do_asort");
			for (i = 0; i < n; i++) {
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}
			qsort((void *)ap, n, sizeof(a_rec),
			      (int (*)(const void *, const void *))arec_comp);
			Get_Poslist(ap, n, poslist);
			XFREE(ap, "do_asort");
		}
		break;
	case NOCASE_LIST:
		if (!listpos_only) {
		        qsort((void *)s, n, sizeof(char *),
			      (int (*)(const void *, const void *))c_comp);
		} else {
			ap = (a_rec *) XCALLOC(n, sizeof(a_rec), "do_asort");
			for (i = 0; i < n; i++) {
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}
			qsort((void *)ap, n, sizeof(a_rec),
			      (int (*)(const void *, const void *))crec_comp);
			Get_Poslist(ap, n, poslist);
			XFREE(ap, "do_asort");
		}
		break;
	case NUMERIC_LIST:
		ip = (i_rec *) XCALLOC(n, sizeof(i_rec), "do_asort");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = atoi(s[i]);
			ip[i].pos = i + 1;
		}
		qsort((void *)ip, n, sizeof(i_rec), 
			(int (*)(const void *, const void *))i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		if (listpos_only) {
			Get_Poslist(ip, n, poslist);
		}
		XFREE(ip, "do_asort");
		break;
	case DBREF_LIST:
		ip = (i_rec *) XCALLOC(n, sizeof(i_rec), "do_asort.2");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
			ip[i].pos = i + 1;
		}
		qsort((void *)ip, n, sizeof(i_rec),
			(int (*)(const void *, const void *))i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		if (listpos_only) {
			Get_Poslist(ip, n, poslist);
		}
		XFREE(ip, "do_asort.2");
		break;
	case FLOAT_LIST:
		fp = (f_rec *) XCALLOC(n, sizeof(f_rec), "do_asort.3");
		for (i = 0; i < n; i++) {
			fp[i].str = s[i];
			fp[i].data = aton(s[i]);
			fp[i].pos = i + 1;
		}
		qsort((void *)fp, n, sizeof(f_rec),
			(int (*)(const void *, const void *))f_comp);
		for (i = 0; i < n; i++) {
			s[i] = fp[i].str;
		}
		if (listpos_only) {
			Get_Poslist(fp, n, poslist);
		}
		XFREE(fp, "do_asort.3");
		break;
	}
	return poslist;
}

FUNCTION(handle_sort)
{
	int nitems, sort_type, oper, i;
	char *list, **ptrs;
	int *poslist;
	Delim isep, osep;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}

	VaChk_In_Out(1, 4);
	oper = Func_Mask(SORT_POS);

	/* Convert the list to an array */

	list = alloc_lbuf("handle_sort");
	strcpy(list, fargs[0]);
	nitems = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);
	sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
	poslist = do_asort(ptrs, nitems, sort_type, oper);
	if (oper == SORT_POS) {
		for (i = 0; i < nitems; i++) {
			if (i > 0) {
				print_sep(&osep, buff, bufc);
			}
			safe_ltos(buff, bufc, poslist[i]);
		}
	} else {
		arr2list(ptrs, nitems, buff, bufc, &osep);
	}
	if (poslist) {
		XFREE(poslist, "do_asort.poslist");
	}
	free_lbuf(list);
	XFREE(ptrs, "handle_sort.ptrs");
}

/* ---------------------------------------------------------------------------
 * sortby: Sort using a user-defined function.
 */

static char ucomp_buff[LBUF_SIZE];
static dbref ucomp_cause, ucomp_player, ucomp_caller;

static int u_comp(s1, s2)
const void *s1, *s2;
{
	/* Note that this function is for use in conjunction with our own 
	 * sane_qsort routine, NOT with the standard library qsort! 
	 */

	char *result, *tbuf, *elems[2], *bp, *str;
	int n;

	if ((mudstate.func_invk_ctr > mudconf.func_invk_lim) ||
	    (mudstate.func_nest_lev > mudconf.func_nest_lim) ||
	    Too_Much_CPU())
		return 0;

	tbuf = alloc_lbuf("u_comp");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	strcpy(tbuf, ucomp_buff);
	result = bp = alloc_lbuf("u_comp");
	str = tbuf;
	exec(result, &bp, ucomp_player, ucomp_caller, ucomp_cause,
	     EV_STRIP | EV_FCHECK | EV_EVAL, &str, elems, 2);
	n = atoi(result);
	free_lbuf(result);
	free_lbuf(tbuf);
	return n;
}

static void sane_qsort(array, left, right, compare)
void *array[];
int left, right;
int (*compare) ();
{
	/* Andrew Molitor's qsort, which doesn't require transitivity between
	 * comparisons (essential for preventing crashes due to
	 * boneheads who write comparison functions where a > b doesn't
	 * mean b < a).  
	 */

	int i, last;
	void *tmp;

      loop:
	if (left >= right)
		return;

	/* Pick something at random at swap it into the leftmost slot */
	/* This is the pivot, we'll put it back in the right spot later */

	i = Randomize(1 + (right - left));
	tmp = array[left + i];
	array[left + i] = array[left];
	array[left] = tmp;

	last = left;
	for (i = left + 1; i <= right; i++) {

		/* Walk the array, looking for stuff that's less than our */
		/* pivot. If it is, swap it with the next thing along */

		if ((*compare) (array[i], array[left]) < 0) {
			last++;
			if (last == i)
				continue;

			tmp = array[last];
			array[last] = array[i];
			array[i] = tmp;
		}
	}

	/* Now we put the pivot back, it's now in the right spot, we never */
	/* need to look at it again, trust me.                             */

	tmp = array[last];
	array[last] = array[left];
	array[left] = tmp;

	/* At this point everything underneath the 'last' index is < the */
	/* entry at 'last' and everything above it is not < it.          */

	if ((last - left) < (right - last)) {
		sane_qsort(array, left, last - 1, compare);
		left = last + 1;
		goto loop;
	} else {
		sane_qsort(array, last + 1, right, compare);
		right = last - 1;
		goto loop;
	}
}

FUNCTION(fun_sortby)
{
	char *atext, *list, **ptrs;
	Delim isep, osep;
	int nptrs, aflags, alen, anum;
	dbref thing, aowner;
	ATTR *ap;

	if ((nfargs == 0) || !fargs[0] || !*fargs[0]) {
		return;
	}
	VaChk_Only_In_Out(4);

	Get_Ulambda(player, thing, fargs[0],
                    anum, ap, atext, aowner, aflags, alen);

	strcpy(ucomp_buff, atext);
	ucomp_player = thing;
	ucomp_caller = player;
	ucomp_cause = cause;

	list = alloc_lbuf("fun_sortby");
	strcpy(list, fargs[1]);
	nptrs = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);

	if (nptrs > 1)		/* pointless to sort less than 2 elements */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp);

	arr2list(ptrs, nptrs, buff, bufc, &osep);
	free_lbuf(list);
	free_lbuf(atext);
	XFREE(ptrs, "fun_sortby.ptrs");
}

/* ---------------------------------------------------------------------------
 * handle_sets: Set management: SETUNION, SETDIFF, SETINTER.
 * Also LUNION, LDIFF, LINTER: Same thing, but takes a sort type like
 * sort() does. There's an unavoidable PennMUSH conflict, as
 * setunion() and friends have a 4th-arg output delimiter in TM3, but
 * the 4th arg is used for the sort type in PennMUSH. Also, adding the
 * sort type as a fifth arg for setunion(), etc. would be confusing,
 * since the last two args are, by convention, delimiters. So we add
 * new funcs.
 */

#define NUMCMP(f1,f2) \
	((f1 > f2) ? 1 : ((f1 < f2) ? -1 : 0))

#define GENCMP(x1,x2,s) \
((s == ALPHANUM_LIST) ? strcmp(ptrs1[x1],ptrs2[x2]) : \
 ((s == NOCASE_LIST) ? strcasecmp(ptrs1[x1],ptrs2[x2]) : \
  ((s == FLOAT_LIST) ? NUMCMP(fp1[x1],fp2[x2]) : NUMCMP(ip1[x1],ip2[x2]))))

FUNCTION(handle_sets)
{
	Delim isep, osep;
	int oper, type_arg;
	char *list1, *list2, *oldp, *bb_p;
	char **ptrs1, **ptrs2;
	int i1, i2, n1, n2, val, sort_type;
	int *ip1, *ip2;
	double *fp1, *fp2;

	oper = Func_Mask(SET_OPER);
	type_arg = Func_Mask(SET_TYPE);

	if (type_arg) {
		VaChk_In_Out(2, 5);
	} else {
		VaChk_Only_In_Out(4);
	}

	list1 = alloc_lbuf("fun_setunion.1");
	strcpy(list1, fargs[0]);
	n1 = list2arr(&ptrs1, LBUF_SIZE, list1, &isep);

	list2 = alloc_lbuf("fun_setunion.2");
	strcpy(list2, fargs[1]);
	n2 = list2arr(&ptrs2, LBUF_SIZE, list2, &isep);

	if (type_arg) {
	    if (*fargs[0])
	        sort_type = get_list_type(fargs, nfargs, 3, ptrs1, n1);
            else
	        sort_type = get_list_type(fargs, nfargs, 3, ptrs2, n2);
	} else {
	    sort_type = ALPHANUM_LIST;
	}
	do_asort(ptrs1, n1, sort_type, SORT_ITEMS);
	do_asort(ptrs2, n2, sort_type, SORT_ITEMS);

	/* This conversion is inefficient, since it's already happened
	 * once in do_asort().
	 */

	ip1 = ip2 = NULL;
	fp1 = fp2 = NULL;

	if (sort_type == NUMERIC_LIST) {
	    ip1 = (int *) XCALLOC(n1, sizeof(int), "handle_sets.n1");
	    ip2 = (int *) XCALLOC(n2, sizeof(int), "handle_sets.n2");
	    for (val = 0; val < n1; val++)
		ip1[val] = atoi(ptrs1[val]);
	    for (val = 0; val < n2; val++)
		ip2[val] = atoi(ptrs2[val]);
	} else if (sort_type == DBREF_LIST) {
	    ip1 = (int *) XCALLOC(n1, sizeof(int), "handle_sets.n1");
	    ip2 = (int *) XCALLOC(n2, sizeof(int), "handle_sets.n2");
	    for (val = 0; val < n1; val++)
		ip1[val] = dbnum(ptrs1[val]);
	    for (val = 0; val < n2; val++)
		ip2[val] = dbnum(ptrs2[val]);
	} else if (sort_type == FLOAT_LIST) {
	    fp1 = (double *) XCALLOC(n1, sizeof(double), "handle_sets.n1");
	    fp2 = (double *) XCALLOC(n2, sizeof(double), "handle_sets.n2");
	    for (val = 0; val < n1; val++)
		fp1[val] = aton(ptrs1[val]);
	    for (val = 0; val < n2; val++)
		fp2[val] = aton(ptrs2[val]);
	}

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
				        print_sep(&osep, buff, bufc);
				}
				oldp = *bufc;
				if (GENCMP(i1, i2, sort_type) < 0) {
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
					print_sep(&osep, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}
		for (; i2 < n2; i2++) {
			if (strcmp(oldp, ptrs2[i2])) {
			        if (*bufc != bb_p) {
				    print_sep(&osep, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs2[i2], buff, bufc);
				**bufc = '\0';
			}
		}
		break;
	case SET_INTERSECT:	/* Copy elements not in both lists */

		while ((i1 < n1) && (i2 < n2)) {
			val = GENCMP(i1, i2, sort_type);
			if (!val) {

				/* Got a match, copy it */

			        if (*bufc != bb_p) {
					print_sep(&osep, buff, bufc);
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
			val = GENCMP(i1, i2, sort_type);
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
					print_sep(&osep, buff, bufc);
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
			    print_sep(&osep, buff, bufc);
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
	if ((sort_type == NUMERIC_LIST) || (sort_type == DBREF_LIST)) {
	    XFREE(ip1, "handle_sets.n1");
	    XFREE(ip2, "handle_sets.n2");
	} else if (sort_type == FLOAT_LIST) {
	    XFREE(fp1, "handle_sets.n1");
	    XFREE(fp2, "handle_sets.n2");
	}
	XFREE(ptrs1, "handle_sets.ptrs1");
	XFREE(ptrs2, "handle_sets.ptrs2");
}

/*---------------------------------------------------------------------------
 * Format a list into columns.
 */

FUNCTION(fun_columns)
{
	unsigned int spaces, number, ansinumber, striplen;
	unsigned int count, i, indent = 0;
	int isansi = 0, rturn = 1;
	char *p, *q, *buf, *objstring, *cp, *cr = NULL;
	Delim isep;

	VaChk_Range(2, 4);
	VaChk_InSep(3, 0);
		
	number = (unsigned int) safe_atoi(fargs[1]);
	indent = (unsigned int) safe_atoi(fargs[3]);

	if (indent > 77) {	/* unsigned int, always a positive number */
		indent = 1;
	}

	/* Must check number separately, since number + indent can
	 * result in an integer overflow.
	 */

	if ((number < 1) || (number > 77) ||
	    ((unsigned int) (number + indent) > 78)) {
		safe_str("#-1 OUT OF RANGE", buff, bufc);
		return;
	}

	cp = trim_space_sep(fargs[0], &isep);
	if (!*cp) {
		return;
	}
	
	for (i = 0; i < indent; i++)
		safe_chr(' ', buff, bufc);

	buf = alloc_lbuf("fun_columns");
	
	while (cp) {
		objstring = split_token(&cp, &isep);
		ansinumber = number;
		striplen = strip_ansi_len(objstring);
		if (ansinumber > striplen)
		    ansinumber = striplen;

		p = objstring;
		q = buf;
		count = 0;
		while (p && *p) {
			if (count == number) {
				break;
			}
			if (*p == ESC_CHAR) {
				/* Start of ANSI code. Skip to end. */
				isansi = 1;
				while (*p && !isalpha(*p))
					*q++ = *p++;
				if (*p)
					*q++ = *p++;
			} else {
				*q++ = *p++;
				count++;
			}
		}
		if (isansi)
		    safe_ansi_normal(buf, &q);
		*q = '\0';
		isansi = 0;

		safe_str(buf, buff, bufc);

		if (striplen < number) {

		    /* We only need spaces if we need to pad out.
		     * Sanitize the number of spaces, too. 
		     */

		    spaces = number - striplen;
		    if (spaces > LBUF_SIZE) {
			spaces = LBUF_SIZE;
		    }

		    for (i = 0; i < spaces; i++)
			safe_chr(' ', buff, bufc);
		}

		if (!(rturn % (int)((78 - indent) / number))) {
			safe_crlf(buff, bufc);
			cr = *bufc;
			for (i = 0; i < indent; i++)
				safe_chr(' ', buff, bufc);
		} else {
			cr = NULL;
		}

		rturn++;
	}
	
	if (cr) {
		*bufc = cr;
		**bufc = '\0';
	} else {
		safe_crlf(buff, bufc);
	}
	
	free_lbuf(buf);
}

/*---------------------------------------------------------------------------
 * fun_table: Turn a list into a table.
 *   table(<list>,<field width>,<line length>,<list delim>,<field sep>,<pad>)
 *     Only the <list> parameter is mandatory.
 *   tables(<list>,<field widths>,<lead str>,<trail str>,
 *          <list delim>,<field sep str>,<pad>)
 *     Only the <list> and <field widths> parameters are mandatory. 
 *
 * There are a couple of PennMUSH incompatibilities. The handling here is
 * more complex and probably more desirable behavior. The issues are:
 *   - ANSI states are preserved even if a word is truncated. Thus, the
 *     next word will start with the correct color.
 *   - ANSI does not bleed into the padding or field separators. 
 *   - Having a '%r' embedded in the list will start a new set of columns.
 *     This allows a series of %r-separated lists to be table-ified
 *     correctly, and doesn't mess up the character count.
 */

static void tables_helper(list, last_state, n_cols, col_widths,
			  lead_str, trail_str, list_sep, field_sep, pad_char,
			  buff, bufc, just)
    char *list;
    int *last_state, n_cols, col_widths[];
    char *lead_str, *trail_str;
    const Delim *list_sep, *field_sep, *pad_char;
    char *buff, **bufc;
    int just;
{
    int i, nwords, nstates, cpos, wcount, over, ansi_state;
    int max, nleft, lead_chrs, lens[LBUF_SIZE / 2], states[LBUF_SIZE / 2 + 1];
    char *s, **words;

    /* Split apart the list. We need to find the length of each de-ansified
     * word, as well as keep track of the state of each word.
     * Overly-long words eventually get truncated, but the correct ANSI
     * state is preserved nonetheless.
     */
    nstates = list2ansi(states, last_state, LBUF_SIZE / 2, list, list_sep);
    nwords = list2arr(&words, LBUF_SIZE / 2, list, list_sep);
    if (nstates != nwords) {
	safe_tprintf_str(buff, bufc, "#-1 STATE/WORD COUNT OFF: %d/%d",
			 nstates, nwords);
	XFREE(words, "tables_helper.words");
	return;
    }
    for (i = 0; i < nwords; i++)
	lens[i] = strip_ansi_len(words[i]);

    over = wcount = 0;
    while ((wcount < nwords) && !over) {

	/* Beginning of new line. Insert newline if this isn't the first
	 * thing we're writing. Write left margin, if appropriate.
	 */
	if (wcount != 0)
	    safe_crlf(buff, bufc);

	if (lead_str)
	    over = safe_str_fn(lead_str, buff, bufc);

	/* Do each column in the line. */

	for (cpos = 0; (cpos < n_cols) && (wcount < nwords) && !over;
	     cpos++, wcount++) {

	    /* Write leading padding if we need it. */

	    if (just == JUST_RIGHT) {
		nleft = col_widths[cpos] - lens[wcount];
		print_padding(nleft, max, pad_char->str[0]);
	    } else if (just == JUST_CENTER) {
		lead_chrs = (int)((col_widths[cpos] / 2) -
				  (lens[wcount] / 2) + .5);
		print_padding(lead_chrs, max, pad_char->str[0]);
	    }

	    /* If we had a previous state, we have to write it. */

	    safe_str(ansi_transition_esccode(ANST_NONE, states[wcount]),
		     buff, bufc);

	    /* Copy in the word. */
	    
	    if (lens[wcount] <= col_widths[cpos]) {
		over = safe_str_fn(words[wcount], buff, bufc);
		safe_str(ansi_transition_esccode(states[wcount + 1],
						 ANST_NONE),
			 buff, bufc);
	    } else {
		/* Bleah. We have a string that's too long. Truncate it.
		 * Write an ANSI normal at the end at the end if we need
		 * one (we'll restore the correct ANSI code with the
		 * next word, if need be).
		 */
	        ansi_state = states[wcount];
		for (s = words[wcount], i = 0;
		     *s && (i < col_widths[cpos]); ) {
		    if (*s == ESC_CHAR) {
			track_esccode(s, ansi_state);
		    } else {
			s++;
			i++;
		    }
		}
		safe_known_str(words[wcount], s - words[wcount], buff, bufc);
		safe_str(ansi_transition_esccode(ansi_state, ANST_NONE),
			 buff, bufc);
	    }

	    /* Writing trailing padding if we need it. */
	    
	    if (just & JUST_LEFT) {
		nleft = col_widths[cpos] - lens[wcount];
		print_padding(nleft, max, pad_char->str[0]);
	    } else if (just & JUST_CENTER) {
		nleft = col_widths[cpos] - lead_chrs - lens[wcount];
		print_padding(nleft, max, pad_char->str[0]);
	    }

	    /* Insert the field separator if this isn't the last column
	     * AND this is not the very last word in the list.
	     */

	    if ((cpos < n_cols - 1) && (wcount < nwords - 1)) {
		print_sep(field_sep, buff, bufc);
	    }
	}

	if (!over && trail_str) {

	    /* If we didn't get enough columns to fill out a line, and
	     * this is the last line, then we have to pad it out.
	     */
	    if ((wcount == nwords) &&
		((nleft = nwords % n_cols) > 0)) {
		for (cpos = nleft; (cpos < n_cols) && !over; cpos++) {
		    print_sep(field_sep, buff, bufc);
		    print_padding(col_widths[cpos], max, pad_char->str[0]);
		}
	    }

	    /* Write the right margin. */
	    over = safe_str_fn(trail_str, buff, bufc);
	}
    }

    /* Save the ANSI state of the last word. */
    *last_state = states[nstates - 1];

    /* Clean up. */
    XFREE(words, "tables_helper.words");
}

static void perform_tables(player, list, n_cols, col_widths,
			   lead_str, trail_str, list_sep, field_sep, pad_char,
			   buff, bufc, just)
    dbref player;
    char *list;
    int n_cols, col_widths[];
    char *lead_str, *trail_str;
    const Delim *list_sep, *field_sep, *pad_char;
    char *buff, **bufc;
    int just;
{
    char *p, *savep, *bb_p;
    int ansi_state = ANST_NONE;

    if (!list || !*list)
	return;

    bb_p = *bufc;
    savep = list;
    p = strchr(list, '\r');
    while (p) {
	*p = '\0';
	if (*bufc != bb_p)
	    safe_crlf(buff, bufc);
	tables_helper(savep, &ansi_state, n_cols, col_widths,
		      lead_str, trail_str, list_sep, field_sep, pad_char,
		      buff, bufc, just);
	savep = p + 2;	/* must skip '\n' too */
	p = strchr(savep, '\r'); 
    }
    if (*bufc != bb_p)
	safe_crlf(buff, bufc);
    tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str,
		  list_sep, field_sep, pad_char, buff, bufc, just);
}

FUNCTION(process_tables)
{
    int just;
    int i, num, n_columns, *col_widths;
    Delim list_sep, field_sep, pad_char;
    char **widths;

    just = Func_Mask(JUST_TYPE);

    VaChk_Range(2, 7);
    VaChk_Sep(&list_sep, 5, DELIM_STRING);
    VaChk_Sep(&field_sep, 6, DELIM_STRING|DELIM_NULL|DELIM_CRLF);
    VaChk_Sep(&pad_char, 7, 0);

    n_columns = list2arr(&widths, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);
    if (n_columns < 1) {
	XFREE(widths, "process_tables.widths");
	return;
    }

    col_widths = (int *) XCALLOC(n_columns, sizeof(int), "process_tables.col_widths");
    for (i = 0; i < n_columns; i++) {
	num = atoi(widths[i]);
	col_widths[i] = (num < 1) ? 1 : num;
    }

    perform_tables(player, fargs[0], n_columns, col_widths,
		   ((nfargs > 2) && *fargs[2]) ? fargs[2] : NULL,
		   ((nfargs > 3) && *fargs[3]) ? fargs[3] : NULL, 
		   &list_sep, &field_sep, &pad_char, buff, bufc, just);

    XFREE(col_widths, "process_tables.col_widths");
    XFREE(widths, "process_tables.widths");
}

FUNCTION(fun_table)
{
    int line_length = 78;
    int field_width = 10;
    int just = JUST_LEFT;
    int i, field_sep_width, n_columns, *col_widths;
    char *p;
    Delim list_sep, field_sep, pad_char;

    VaChk_Range(1, 6);
    VaChk_Sep(&list_sep, 4, DELIM_STRING);
    VaChk_Sep(&field_sep, 5, DELIM_STRING|DELIM_NULL|DELIM_CRLF);
    VaChk_Sep(&pad_char, 6, 0);

    /* Get line length and column width. All columns are the same width.
     * Calculate what we need to.
     */

    if (nfargs > 2) {
	line_length = atoi(fargs[2]);
	if (line_length < 2)
	    line_length = 2;
    }

    if (nfargs > 1) {
	p = fargs[1];
	switch (*p) {
	    case '<':
		 just = JUST_LEFT;
		 p++;
		 break;
	    case '>':
		 just = JUST_RIGHT;
		 p++;
		 break;
	    case '-':
		 just = JUST_CENTER;
		 p++;
		 break;
	}
	field_width = atoi(p);
	if (field_width < 1)
	    field_width = 1;
	else if (field_width > LBUF_SIZE - 1)
	    field_width = LBUF_SIZE - 1;
    }

    if (field_width >= line_length)
	field_width = line_length - 1;

    if (field_sep.len == 1) {
	switch (field_sep.str[0]) {
	case '\r':
	case '\0':
	case '\n':
	case '\a':
	    field_sep_width = 0;
	    break;
	default:
	    field_sep_width = 1;
	    break;
	}
    } else {
	field_sep_width = strip_ansi_len(field_sep.str);
    }

    n_columns = (int)(line_length / (field_width + field_sep_width));
    col_widths = (int *) XCALLOC(n_columns, sizeof(int), "fun_table.widths");
    for (i = 0; i < n_columns; i++)
	col_widths[i] = field_width;

    perform_tables(player, fargs[0], n_columns, col_widths, NULL, NULL,
		   &list_sep, &field_sep, &pad_char, buff, bufc, just);

    XFREE(col_widths, "fun_table.widths");
}

/*---------------------------------------------------------------------------
 * fun_align: Turn a set of lists into newspaper-like columns.
 *   align(<widths>,<col1>,...,<colN>,<fill char>,<col sep>,<row sep>)
 *   lalign(<widths>,<col data>,<delim>,<fill char>,<col sep>,<row sep>)
 *   Only <widths> and the column data parameters are mandatory.
 *
 * This is mostly PennMUSH-compatible, but not 100%.
 *   - ANSI is not stripped out of the column text. States will be correctly
 *     preserved, and will not bleed.
 *   - ANSI states are not supported in the widths, as they are unnecessary.
 */

static void perform_align(n_cols, raw_colstrs, data, fillc, col_sep, row_sep,
                          buff, bufc)
int n_cols;
char **raw_colstrs, **data;
char fillc;
Delim col_sep, row_sep;
char *buff, **bufc;
{
     int i, n;
     int *col_widths, *col_justs, *col_done;
     char *p, *bb_p, *l_p;
     char **xsl, **xel, **xsw, **xew;
     int *xsl_a, *xel_a, *xsw_a, *xew_a;
     int *xsl_p, *xel_p, *xsw_p, *xew_p;
     char *sl, *el, *sw, *ew;
     int sl_ansi_state, el_ansi_state, sw_ansi_state, ew_ansi_state;
     int sl_pos, el_pos, sw_pos, ew_pos;
     int width, just, nleft, max, lead_chrs;
     int n_done = 0, pending_coaright = 0; 

     /* Parse the column widths and justifications */

     col_widths = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.widths");
     col_justs = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.justs");
     for (i = 0; i < n_cols; i++) {
	 p = raw_colstrs[i];
	 switch (*p) {
	     case '<':
		  col_justs[i] = JUST_LEFT;
		  p++;
		  break;
	     case '>':
		  col_justs[i] = JUST_RIGHT;
		  p++;
		  break;
	     case '-':
		  col_justs[i] = JUST_CENTER;
		  p++;
		  break;
	     default:
		  col_justs[i] = JUST_LEFT;
	 }
	 for (n = 0; *p && isdigit((unsigned char) *p); p++) {
	     n *= 10;
	     n += *p - '0';
	 }
	 if (n < 1) {
	     safe_str("#-1 INVALID COLUMN WIDTH", buff, bufc);
	     XFREE(col_widths, "perform_align.widths");
	     XFREE(col_justs, "perform_align.justs");
	     return;
	 }
	 col_widths[i] = n;
	 switch (*p) {
	     case '.':
		  col_justs[i] |= JUST_REPEAT;
		  p++;
		  break;
	     case '`':
		  col_justs[i] |= JUST_COALEFT;
		  p++;
		  break;
	     case '\'':
		  col_justs[i] |= JUST_COARIGHT;
		  p++;
		  break;
	 }
	 if (*p) {
	     safe_str("#1 INVALID ALIGN STRING", buff, bufc);
	     XFREE(col_widths, "perform_align.widths");
	     XFREE(col_justs, "perform_align.justs");
	     return;
	 }
     }

     col_done = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.done");
     xsl = (char **) XCALLOC(n_cols, sizeof(char *), "perform_align.xsl");
     xel = (char **) XCALLOC(n_cols, sizeof(char *), "perform_align.xel");
     xsw = (char **) XCALLOC(n_cols, sizeof(char *), "perform_align.xsw");
     xew = (char **) XCALLOC(n_cols, sizeof(char *), "perform_align.xew");
     xsl_a = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xsl_a");
     xel_a = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xel_a");
     xsw_a = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xsw_a");
     xew_a = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xew_a");
     xsl_p = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xsl_p");
     xel_p = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xel_p");
     xsw_p = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xsw_p");
     xew_p = (int *) XCALLOC(n_cols, sizeof(int), "perform_align.xew_p");

     /* calloc() auto-initializes things to 0, so just do the other things */

     for (i = 0; i < n_cols; i++) {
	 xew[i] = data[i];
	 xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
     }

     bb_p = *bufc;
     l_p = *bufc;

     while (n_done < n_cols) {
	 for (i = 0; i < n_cols; i++) {

	     /* If this is the first column, and it's not our first line,
	      * output a row separator.
	      */
	     if ((i == 0) && (*bufc != bb_p)) {
		 print_sep(&row_sep, buff, bufc);
		 l_p = *bufc;
	     }
	     
	     /* If our column width is 0, we've coalesced and we can safely
	      * continue.
	      */
	     if (col_widths[i] == 0)
		 continue;

	     /* If this is not the first column of this line, output a
	      * column separator.
	      */
	     if (*bufc != l_p) {
		 print_sep(&col_sep, buff, bufc);
	     }

	     /* If we have a pending right-coalesce, we take care of it now,
	      * though we save our previous width at this stage, since we're
	      * going to output at that width during this pass.
	      * We know we're not at width 0 ourselves at this point, so we
	      * don't have to worry about a cascading coalesce; it'll 
	      * get taken care of by the loop, automatically.
	      * If we have a pending coalesce-right and we're at the first
	      * column, we know that we overflowed and should just clear it.
	      */

	     width = col_widths[i];
	     if (pending_coaright) {
		 if (i > 0)
		     col_widths[i] += pending_coaright + col_sep.len;
		 pending_coaright = 0;
	     }

	     /* If we're done and our column width is not zero, and we are
	      * not repeating, we must fill in spaces before we continue.
	      */

	     if (col_done[i] && !(col_justs[i] & JUST_REPEAT)) {
		 print_padding(width, max, fillc);
		 continue;
	     }

	     /* Restore our state variables */

	     sl = xsl[i]; el = xel[i]; sw = xsw[i]; ew = xew[i]; 
	     sl_ansi_state = xsl_a[i];
	     el_ansi_state = xel_a[i];
	     sw_ansi_state = xsw_a[i];
	     ew_ansi_state = xew_a[i];
	     sl_pos = xsl_p[i];
	     el_pos = xel_p[i];
	     sw_pos = xsw_p[i];
	     ew_pos = xew_p[i];
	     just = col_justs[i];

	     while (1) {
		 /* Locate the next start-of-word (SW) */
		 for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos;
		      *sw; ++sw) {
		     switch(*sw) {
			 case ESC_CHAR:
			      track_esccode(sw, sw_ansi_state);
			      --sw;
			      continue;
			 case '\t':
			 case '\r':
			      *sw = ' ';
			      /* FALLTHRU */
			 case ' ':
			      ++sw_pos;
			      continue;
			 case BEEP_CHAR:
			      continue;
			 default:
			      break;
		     }
		     break;
		 }

		 /* Three ways out of that locator loop: end-of-string (ES),
		  * end-of-line (EL), and start-of-word (SW)
		  */

		 if (!*sw && sl == NULL) { /* ES, and nothing left to output */
		     /* If we're coalescing left, we set this column to 0
		      * width, and increase the width of the left column.
		      * If we're coalescing right, we can't widen that column
		      * yet, because otherwise it'll throw off its width for
		      * this pass, so we've got to do that later. 
		      * If we're repeating, we reset our pointer state, but
		      * we keep track of our done-ness. Don't increment done
		      * more than once, since we might repeat several times.
		      */
		     if (!col_done[i]) {
			 n_done++;
			 col_done[i] = 1;
		     }
		     if (i && (just & JUST_COALEFT)) {
			 /* Find the next-left column with a nonzero width,
			  * since we can have casdading coalescing.
			  */
			 for (n = i - 1;
			      (n > 0) && (col_widths[n] == 0);
			      n--);
			 /* We have to add not only the width of the column,
			  * but the column separator length.
			  */
			 col_widths[n] += col_widths[i] + col_sep.len;
			 col_widths[i] = 0;
		     } else if ((just & JUST_COARIGHT) && (i + 1 < n_cols)) {
			 pending_coaright = col_widths[i];
			 col_widths[i] = 0;
		     } else if (just & JUST_REPEAT) {
			 xsl[i] = xel[i] = xsw[i] = NULL;
			 xew[i] = data[i];
			 xsl_a[i] = xel_a[i] = ANST_NORMAL;
			 xsw_a[i] = xew_a[i] = ANST_NORMAL;
			 xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
		     }
		     break;	/* get out of our infinite while */
		 }

		 /* Decide where start-of-line (SL) was */
		 if (sl == NULL) {
		     if (ew == data[i] || ew[-1] == '\n') {
			 /* Preserve indentation at SS or after explicit EL */
			 sl = ew;
			 sl_ansi_state = ew_ansi_state;
			 sl_pos = ew_pos;
		     } else {
			 /* Discard whitespace if previous line wrapped */
			 sl = sw;
			 sl_ansi_state = sw_ansi_state;
			 sl_pos = sw_pos;
		     }
		 }

		 if (*sw == '\n') { /* EL, so we have to output */
		     ew = sw;
		     ew_ansi_state = sw_ansi_state;
		     ew_pos = sw_pos;
		     break;
		 } else {
		     /* Locate the next end-of-word (EW) */
		     for (ew = sw, ew_ansi_state = sw_ansi_state, ew_pos = sw_pos;
			  *ew; ++ew) {
			 switch(*ew) {
			     case ESC_CHAR:
				  track_esccode(ew, ew_ansi_state);
				  --ew;
				  continue;
			     case '\r':
			     case '\t':
				  *ew = ' ';
				  /* FALLTHRU */
			     case ' ':
			     case '\n':
				  break;
			     case BEEP_CHAR:
				  continue;
			     default:
				  /* Break up long words */
				  if (ew_pos - sw_pos == width)
				      break;
				  ++ew_pos;
				  continue;
			 }
			 break;
		     }

		     /* Three ways out of that previous for loop: ES, EL, EW */

		     /* If it fits on the line, add it */
		     if (ew_pos - sl_pos <= width) {
			 el = ew;
			 el_ansi_state = ew_ansi_state;
			 el_pos = ew_pos;
		     }

		     /* If it's just EW, not ES or EL, and the line isn't too
		      * long, keep adding words to the line
		      */
		     if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
			 continue;

		     /* So now we definitely need to output a line */
		     break;
		 }
	     }

	     /* Could be a blank line, no words fit */
	     if (el == NULL) {
		 el = sw;
		 el_ansi_state = sw_ansi_state;
		 el_pos = sw_pos;
	     }

	     /* Left space padding if needed */
	     if (just & JUST_RIGHT) {
		 nleft = width - el_pos + sl_pos;
		 print_padding(nleft, max, fillc);
	     } else if (just & JUST_CENTER) {
		 lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);
		 print_padding(lead_chrs, max, fillc);
	     }

	     /* Restore previous ansi state */
	     safe_str(ansi_transition_esccode(ANST_NORMAL, sl_ansi_state),
		      buff, bufc);

	     /* Print the words */
	     safe_known_str(sl, el - sl, buff, bufc);

	     /* Back to ansi normal */
	     safe_str(ansi_transition_esccode(el_ansi_state, ANST_NORMAL),
		      buff, bufc);

	     /* Right space padding if needed */
	     if (just & JUST_LEFT) {
		 nleft = width - el_pos + sl_pos;
		 print_padding(nleft, max, fillc);
	     } else if (just & JUST_CENTER) {
		 nleft = width - lead_chrs - el_pos + sl_pos;
		 print_padding(nleft, max, fillc);
	     }

	     /* Update pointers for the next line */

	     if (!*el) {
		 /* ES, and nothing left to output */
		 if (!col_done[i]) {
		     n_done++;
		     col_done[i] = 1;
		 }
		 if ((just & JUST_COALEFT) && (i - 1 >= 0)) {
		     for (n = i - 1;
			  (n > 0) && (col_widths[n] == 0);
			  n--);
		     col_widths[n] += col_widths[i] + col_sep.len;
		     col_widths[i] = 0;
		 } else if ((just & JUST_COARIGHT) && (i + 1 < n_cols)) {
		     pending_coaright = col_widths[i];
		     col_widths[i] = 0;
		 } else if (just & JUST_REPEAT) {
		     xsl[i] = xel[i] = xsw[i] = NULL;
		     xew[i] = data[i];
		     xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
		     xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
		 }
	     } else {
		 if (*ew == '\n' && sw == ew) {
		     /* EL already handled on this line, and no new word yet */
		     ++ew;
		     sl = el = NULL;
		 } else if (sl == sw) {
		     /* No new word yet */
		     sl = el = NULL;
		 } else {
		     /* ES with more to output, EL for next line, or just a
			full line */
		     sl = sw;
		     sl_ansi_state = sw_ansi_state;
		     sl_pos = sw_pos;
		     el = ew;
		     el_ansi_state = ew_ansi_state;
		     el_pos = ew_pos;
		 }
		 /* Save state */
		 xsl[i] = sl; xel[i] = el; xsw[i] = sw; xew[i] = ew;
		 xsl_a[i] = sl_ansi_state;
		 xel_a[i] = el_ansi_state;
		 xsw_a[i] = sw_ansi_state;
		 xew_a[i] = ew_ansi_state;
		 xsl_p[i] = sl_pos;
		 xel_p[i] = el_pos;
		 xsw_p[i] = sw_pos;
		 xew_p[i] = ew_pos;
	     }
	 }
     }

     XFREE(col_widths, "perform_align.widths");
     XFREE(col_justs, "perform_align.justs");
     XFREE(col_done, "perform_align.done");
     XFREE(xsl, "perform_align.xsl");
     XFREE(xel, "perform_align.xel");
     XFREE(xsw, "perform_align.xsw");
     XFREE(xew, "perform_align.xew");
     XFREE(xsl_a, "perform_align.xsl_a");
     XFREE(xel_a, "perform_align.xel_a");
     XFREE(xsw_a, "perform_align.xsw_a");
     XFREE(xew_a, "perform_align.xew_a");
     XFREE(xsl_p, "perform_align.xsl_p");
     XFREE(xel_p, "perform_align.xel_p");
     XFREE(xsw_p, "perform_align.xsw_p");
     XFREE(xew_p, "perform_align.xew_p");
}

FUNCTION(fun_align)
{
     char **raw_colstrs;
     int n_cols;
     Delim filler, col_sep, row_sep;

     if (nfargs < 2) {
	 safe_str("#-1 FUNCTION (ALIGN) EXPECTS AT LEAST 2 ARGUMENTS",
		  buff, bufc);
	 return;
     }

     /* We need to know how many columns we have, so we know where the
      * column arguments stop and where the optional arguments start.
      */

     n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);
     if (nfargs < n_cols + 1) {
	 safe_str("#-1 NOT ENOUGH COLUMNS FOR ALIGN", buff, bufc);
	 XFREE(raw_colstrs, "fun_align.raw_colstrs");
	 return;
     }
     if (nfargs > n_cols + 4) {
	 safe_str("#-1 TOO MANY COLUMNS FOR ALIGN", buff, bufc);
	 XFREE(raw_colstrs, "fun_align.raw_colstrs");
	 return;
     }

     /* Note that the VaChk macros number arguments from 1. */
     VaChk_Sep(&filler, n_cols + 2, 0);
     VaChk_SepOut(col_sep, n_cols + 3, 0);
     VaChk_SepOut(row_sep, n_cols + 4, 0);
     if (nfargs < n_cols + 4) {
	 row_sep.str[0] = '\r';
     }

     perform_align(n_cols, raw_colstrs, fargs + 1, filler.str[0],
		   col_sep, row_sep, buff, bufc);
     XFREE(raw_colstrs, "fun_align.raw_colstrs");
}

FUNCTION(fun_lalign)
{
     char **raw_colstrs, **data;
     int n_cols, n_data;
     Delim isep, filler, col_sep, row_sep;

     VaChk_Range(2, 6);

     n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);
     VaChk_InSep(3, 0);
     n_data = list2arr(&data, LBUF_SIZE / 2, fargs[1], &isep); 
     if (n_cols > n_data) {
	 safe_str("#-1 NOT ENOUGH COLUMNS FOR LALIGN", buff, bufc);
	 XFREE(raw_colstrs, "fun_lalign.raw_colstrs");
	 XFREE(data, "fun_lalign.data");
	 return;
     }
     if (n_cols < n_data) {
	 safe_str("#-1 TOO MANY COLUMNS FOR LALIGN", buff, bufc);
	 XFREE(raw_colstrs, "fun_lalign.raw_colstrs");
	 XFREE(data, "fun_lalign.data");
	 return;
     }

     VaChk_Sep(&filler, 4, 0);
     VaChk_SepOut(col_sep, 5, 0);
     VaChk_SepOut(row_sep, 6, 0);
     if (nfargs < 6) {
	 row_sep.str[0] = '\r';
     }

     perform_align(n_cols, raw_colstrs, data, filler.str[0],
		   col_sep, row_sep, buff, bufc);
     XFREE(raw_colstrs, "fun_lalign.raw_colstrs");
     XFREE(data, "fun_lalign.data");
}


/* ---------------------------------------------------------------------------
 * fun_elements: given a list of numbers, get corresponding elements from
 * the list.  elements(ack bar eep foof yay,2 4) ==> bar foof
 * The function takes a separator, but the separator only applies to the
 * first list.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_elements)
{
	int nwords, cur;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	Delim isep, osep;

	VaChk_Only_In_Out(4);
	oldp = *bufc;

	/* Turn the first list into an array. */

	wordlist = alloc_lbuf("fun_elements.wordlist");
	strcpy(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);

	s = Eat_Spaces(fargs[1]);

	/* Go through the second list, grabbing the numbers and finding the
	 * corresponding elements. 
	 */

	do {
		r = split_token(&s, &SPACE_DELIM);
		cur = atoi(r) - 1;
		if ((cur >= 0) && (cur < nwords) && ptrs[cur]) {
		    if (oldp != *bufc) {
			print_sep(&osep, buff, bufc);
		    }
		    safe_str(ptrs[cur], buff, bufc);
		}
	} while (s);
	free_lbuf(wordlist);
	XFREE(ptrs, "fun_elements.ptrs");
}

/* ---------------------------------------------------------------------------
 * fun_grab: a combination of extract() and match(), sortof. We grab the
 *           single element that we match.
 *
 *   grab(Test:1 Ack:2 Foof:3,*:2)    => Ack:2
 *   grab(Test-1+Ack-2+Foof-3,*o*,+)  => Ack:2
 *
 * fun_graball: Ditto, but like matchall() rather than match(). We
 *              grab all the elements that match, and we can take
 *              an output delimiter.
 */

FUNCTION(fun_grab)
{
	char *r, *s;
	Delim isep;

	VaChk_Only_In(3);

	/* Walk the wordstring, until we find the word we want. */

	s = trim_space_sep(fargs[0], &isep);
	do {
		r = split_token(&s, &isep);
		if (quick_wild(fargs[1], r)) {
			safe_str(r, buff, bufc);
			return;
		}
	} while (s);
}

FUNCTION(fun_graball)
{
    char *r, *s, *bb_p;
    Delim isep, osep;

    VaChk_Only_In_Out(4);

    s = trim_space_sep(fargs[0], &isep);
    bb_p = *bufc;
    do {
	r = split_token(&s, &isep);
	if (quick_wild(fargs[1], r)) {
	    if (*bufc != bb_p) {
		print_sep(&osep, buff, bufc);
	    }
	    safe_str(r, buff, bufc);
	}
    } while (s);
}

/* ---------------------------------------------------------------------------
 * fun_shuffle: randomize order of words in a list.
 */

/* Borrowed from PennMUSH 1.50 */
static void swap(p, q)
char **p;
char **q;
{
	/* swaps two points to strings */

	char *temp;

	temp = *p;
	*p = *q;
	*q = temp;
}

FUNCTION(fun_shuffle)
{
	char **words;
	int n, i, j;
	Delim isep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0]) {
		return;
	}
	VaChk_Only_In_Out(3);

	n = list2arr(&words, LBUF_SIZE, fargs[0], &isep);

	for (i = 0; i < n; i++) {
		j = random_range(i, n - 1);
		swap(&words[i], &words[j]);
	}
	arr2list(words, n, buff, bufc, &osep);
	XFREE(words, "fun_shuffle.words");
}

/* ---------------------------------------------------------------------------
 * ledit(<list of words>,<old words>,<new words>[,<delim>[,<output delim>]])
 * If a <word> in <list of words> is in <old words>, replace it with the
 * corresponding word from <new words>. This is basically a mass-edit.
 * This is an EXACT, not a case-insensitive or wildcarded, match.
 */

FUNCTION(fun_ledit)
{
    Delim isep, osep;
    char *old_list, *new_list;
    int nptrs_old, nptrs_new;
    char **ptrs_old, **ptrs_new;
    char *r, *s, *bb_p;
    int i;
    int got_it;

    VaChk_Only_In_Out(5);

    old_list = alloc_lbuf("fun_ledit.old");
    new_list = alloc_lbuf("fun_ledit.new");
    strcpy(old_list, fargs[1]);
    strcpy(new_list, fargs[2]);
    nptrs_old = list2arr(&ptrs_old, LBUF_SIZE / 2, old_list, &isep);
    nptrs_new = list2arr(&ptrs_new, LBUF_SIZE / 2, new_list, &isep);

    /* Iterate through the words. */

    bb_p = *bufc;
    s = trim_space_sep(fargs[0], &isep);
    do {
	if (*bufc != bb_p) {
	    print_sep(&osep, buff, bufc);
	}
	r = split_token(&s, &isep);
	for (i = 0, got_it = 0; i < nptrs_old; i++) {
	    if (!strcmp(r, ptrs_old[i])) {
		got_it = 1;
		if ((i < nptrs_new) && *ptrs_new[i]) {
		    /* If we specify more old words than we have new words,
		     * we assume we want to just nullify.
		     */
		    safe_str(ptrs_new[i], buff, bufc);
		}
		break;
	    }
	}
	if (!got_it) {
	    safe_str(r, buff, bufc);
	}
    } while (s);

    free_lbuf(old_list);
    free_lbuf(new_list);
    XFREE(ptrs_old, "fun_ledit.ptrs_old");
    XFREE(ptrs_new, "fun_ledit.ptrs_new");
}

/* ---------------------------------------------------------------------------
 * fun_itemize: Turn a list into a punctuated list.
 */

FUNCTION(fun_itemize)
{
    Delim isep, osep;
    int n_elems, i;
    char *conj_str, **elems;

    VaChk_Range(1, 4);
    if (!fargs[0] || !*fargs[0])
	return;
    VaChk_InSep(2, 0);
    if (nfargs < 3) {
	conj_str = (char *) "and";
    } else {
	conj_str = fargs[2];
    }
    if (nfargs < 4) {
	osep.str[0] = ',';
	osep.len = 1;
    } else {
	VaChk_OutSep(4, 0);
    }

    n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
    if (n_elems == 1) {
	safe_str(elems[0], buff, bufc);
    } else if (n_elems == 2) {
	safe_str(elems[0], buff, bufc);
	if (*conj_str) {
	    safe_chr(' ', buff, bufc);
	    safe_str(conj_str, buff, bufc);
	}
	safe_chr(' ', buff, bufc);
	safe_str(elems[1], buff, bufc);
    } else {
	for (i = 0; i < (n_elems - 1); i++) {
	    safe_str(elems[i], buff, bufc);
	    print_sep(&osep, buff, bufc);
	    safe_chr(' ', buff, bufc);
	}
	if (*conj_str) {
	    safe_str(conj_str, buff, bufc);
	    safe_chr(' ', buff, bufc);
	}
	safe_str(elems[i], buff, bufc);
    }
    XFREE(elems, "fun_itemize.elems");
}

/* ---------------------------------------------------------------------------
 * fun_choose: Weighted random choice from a list.
 *             choose(<list of items>,<list of weights>,<input delim>)
 */

FUNCTION(fun_choose)
{
    Delim isep;
    char **elems, **weights;
    int i, num, n_elems, n_weights, *ip;
    int sum = 0;

    VaChk_Only_In(3);

    n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
    n_weights = list2arr(&weights, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

    if (n_elems != n_weights) {
	safe_str("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
	XFREE(elems, "fun_choose.elems");
	XFREE(weights, "fun_choose.weights");
	return;
    }

    /* Store the breakpoints, not the choose weights themselves. */

    ip = (int *) XCALLOC(n_weights, sizeof(int), "fun_choose.ip");
    for (i = 0; i < n_weights; i++) {
	num = atoi(weights[i]);
	if (num < 0)
	    num = 0;
	if (num == 0) {
	    ip[i] = 0;
	} else {
	    sum += num;
	    ip[i] = sum;
	}
    }

    num = (int) Randomize(sum);

    for (i = 0; i < n_weights; i++) {
	if ((ip[i] != 0) && (num < ip[i])) {
	    safe_str(elems[i], buff, bufc);
	    break;
	}
    }

    XFREE(ip, "fun_choose.ip");
    XFREE(elems, "fun_choose.elems");
    XFREE(weights, "fun_choose.weights");
}

/* ---------------------------------------------------------------------------
 * fun_group:  group(<list>, <number of groups>, <idelim>, <odelim>, <gdelim>)
 * Sort a list by numerical-size group, i.e., take every Nth
 * element. Useful for passing to a column-type function where you want
 * the list to go down rather than across, for instance.
 */

FUNCTION(fun_group)
{
    char *bb_p, **elems;
    Delim isep, osep, gsep;
    int n_elems, n_groups, i, j;

    /* Separator checking is weird in this, since we can delimit by group,
     * too, as well as the element delimiter. The group delimiter defaults
     * to the output delimiter.
     */

    VaChk_Range(2, 5);
    VaChk_InSep(3, 0);
    VaChk_DefaultOut(4) {
	VaChk_OutSep(4, 0);
    }
    if (nfargs < 5) {
	Delim_Copy(&gsep, &osep);
    } else {
	VaChk_Sep(&gsep, 5, DELIM_NULL|DELIM_CRLF|DELIM_STRING);
    }

    /* Go do it, unless the group size doesn't make sense. */

    n_groups = atoi(fargs[1]);
    n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
    if (n_groups < 2) {
	arr2list(elems, n_elems, buff, bufc, &osep);
	XFREE(elems, "fun_group.elems");
	return;
    }
    if (n_groups >= n_elems) {
	arr2list(elems, n_elems, buff, bufc, &gsep);
	XFREE(elems, "fun_group.elems");
	return;
    }

    bb_p = *bufc;
    for (i = 0; i < n_groups; i++) {
	for (j = 0; i + j < n_elems; j += n_groups) {
	    if (*bufc != bb_p) {
		if (j == 0) {
		    print_sep(&gsep, buff, bufc);
		} else {
		    print_sep(&osep, buff, bufc);
		}
	    }
	    safe_str(elems[i + j], buff, bufc);
	}
    }

    XFREE(elems, "fun_group.elems");
}
