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
 * fun_words: Returns number of words in a string.
 * Added 1/28/91 Philip D. Wasson
 */

FUNCTION(fun_words)
{
	Delim isep;

	if (nfargs == 0) {
		safe_chr('0', buff, bufc);
		return;
	}
	VaChk_Only_In("WORDS", 2);
	safe_ltos(buff, bufc, countwords(fargs[0], isep.c));
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
	VaChk_Only_In("FIRST", 2);
	s = trim_space_sep(fargs[0], isep.c);	/* leading spaces ... */
	first = split_token(&s, isep.c);
	if (first) {
		safe_str(first, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_rest: Returns all but the first word in a string 
 */

FUNCTION(fun_rest)
{
	char *s, *first;
	Delim isep; 

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In("REST", 2);
	s = trim_space_sep(fargs[0], isep.c);	/* leading spaces ... */
	first = split_token(&s, isep.c);
	if (s) {
		safe_str(s, buff, bufc);
	}
}

/* ---------------------------------------------------------------------------
 * fun_last: Returns last word in a string
 */

FUNCTION(fun_last)
{
	char *s, *last;
	Delim isep;
	int len, i;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In("LAST", 2);
	s = trim_space_sep(fargs[0], isep.c);	/* trim leading spaces */

	/* If we're dealing with spaces, trim off the trailing stuff */

	if (isep.c == ' ') {
		len = strlen(s);
		for (i = len - 1; s[i] == ' '; i--) ;
		if (i + 1 <= len)
			s[i + 1] = '\0';
	}
	last = strrchr(s, isep.c);
	if (last)
		safe_str(++last, buff, bufc);
	else
		safe_str(s, buff, bufc);
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

	VaChk_Only_In("MATCH", 3);

	/* Check each word individually, returning the word number of the 
	 * first one that matches.  If none match, return 0. 
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], isep.c);
	do {
		r = split_token(&s, isep.c);
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
	int wcount, osep_len;
	char *r, *s, *old;
	Delim isep, osep;

	VaChk_Only_In_Out("MATCHALL", 4);

	/* SPECIAL CASE: If there's no output delimiter specified, we use
	 * a space, NOT the delimiter given for the list!
	 */
	if (nfargs < 4) {
	    osep.c = ' ';
	    osep_len = 1;
	}

	old = *bufc;

	/* Check each word individually, returning the word number of all 
	 * that match. If none match, return a null string.
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], isep.c);
	do {
		r = split_token(&s, isep.c);
		if (quick_wild(fargs[1], r)) {
			if (old != *bufc) {
			    print_sep(osep, osep_len, buff, bufc);
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
	Delim isep;

	VaChk_Only_In("EXTRACT", 4);

	s = fargs[0];
	start = atoi(fargs[1]);
	len = atoi(fargs[2]);

	if ((start < 1) || (len < 1)) {
		return;
	}
	/* Skip to the start of the string to save */

	start--;
	s = trim_space_sep(s, isep.c);
	while (start && s) {
		s = next_token(s, isep.c);
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
		s = next_token(s, isep.c);
		len--;
	}

	/* Chop off the rest of the string, if needed */

	if (s && *s)
		t = split_token(&s, isep.c);
	safe_str(r, buff, bufc);
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
 */

#define	IF_DELETE	0
#define	IF_REPLACE	1
#define	IF_INSERT	2

static void do_itemfuns(buff, bufc, str, el, word, isep, flag)
char *buff, **bufc, *str, *word;
Delim isep;
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
			eptr = trim_space_sep(str, isep.c);
			iptr = split_token(&eptr, isep.c);
		}
	} else {
		/* Break off 'before' portion */

		sptr = eptr = trim_space_sep(str, isep.c);
		overrun = 1;
		for (ct = el; ct > 2 && eptr;
		     eptr = next_token(eptr, isep.c), ct--) ;
		if (eptr) {
			overrun = 0;
			iptr = split_token(&eptr, isep.c);
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
			iptr = split_token(&eptr, isep.c);
		else
			iptr = NULL;
	}

	switch (flag) {
	case IF_DELETE:	/* deletion */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			if (eptr)
				safe_chr(isep.c, buff, bufc);
		}
		if (eptr) {
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_REPLACE:	/* replacing */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			safe_chr(isep.c, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (eptr) {
			safe_chr(isep.c, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	case IF_INSERT:	/* insertion */
		if (sptr) {
			safe_str(sptr, buff, bufc);
			safe_chr(isep.c, buff, bufc);
		}
		safe_str(word, buff, bufc);
		if (iptr) {
			safe_chr(isep.c, buff, bufc);
			safe_str(iptr, buff, bufc);
		}
		if (eptr) {
			safe_chr(isep.c, buff, bufc);
			safe_str(eptr, buff, bufc);
		}
		break;
	}
}

FUNCTION(fun_ldelete)
{				/* delete a word at position X of a list */
	Delim isep;

	VaChk_Only_In("LDELETE", 3);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), NULL, isep, IF_DELETE);
}

FUNCTION(fun_replace)
{				/* replace a word at position X of a list */
	Delim isep;

	VaChk_Only_In("REPLACE", 4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], isep, IF_REPLACE);
}

FUNCTION(fun_insert)
{				/* insert a word at position X of a list */
	Delim isep;

	VaChk_Only_In("INSERT", 4);
	do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], isep, IF_INSERT);
}

/* ---------------------------------------------------------------------------
 * fun_remove: Remove a word from a string
 */

FUNCTION(fun_remove)
{
	char *s, *sp, *word;
	Delim isep;
	int first, found;

	VaChk_Only_In("REMOVE", 3);
	if (strchr(fargs[1], isep.c)) {
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
		sp = split_token(&s, isep.c);
		if (found || strcmp(sp, word)) {
			if (!first)
				safe_chr(isep.c, buff, bufc);
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
	char *r, *s;
	Delim isep;

	VaChk_Only_In("MEMBER", 3);
	wcount = 1;
	s = trim_space_sep(fargs[0], isep.c);
	do {
		r = split_token(&s, isep.c);
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
	char *temp, *tp, *t1;
	Delim isep;
	int first;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	VaChk_Only_In("REVWORDS", 2);
	temp = alloc_lbuf("fun_revwords");

	/* Nasty bounds checking */

	if ((int)strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff) - 1) {
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff) - 1)) = '\0';
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
			safe_chr(isep.c, buff, bufc);
		t1 = split_token(&tp, isep.c);
		do_reverse(t1, *bufc);
		*bufc += strlen(t1);
		first = 0;
	}
	free_lbuf(temp);
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
	int words, i, osep_len;

	VaChk_Only_In_Out("SPLICE", 5);

	/* length checks */

	if (countwords(fargs[2], isep.c) > 1) {
		safe_str("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}
	words = countwords(fargs[0], isep.c);
	if (words != countwords(fargs[1], isep.c)) {
		safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}
	/* loop through the two lists */

	p1 = fargs[0];
	q1 = fargs[1];
	bb_p = *bufc;
	for (i = 0; i < words; i++) {
		p2 = split_token(&p1, isep.c);
		q2 = split_token(&q1, isep.c);
		if (*bufc != bb_p) {
		    print_sep(osep, osep_len, buff, bufc);
		}
		if (!strcmp(p2, fargs[2]))
			safe_str(q2, buff, bufc);	/* replace */
		else
			safe_str(p2, buff, bufc);	/* copy */
	}
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

static int c_comp(s1, s2)
const void *s1, *s2;
{
	return strcasecmp(*(char **)s1, *(char **)s2);
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
	f_rec *fp = NULL;
	i_rec *ip = NULL;

	switch (sort_type) {
	case ALPHANUM_LIST:
		qsort((void *)s, n, sizeof(char *), 
			(int (*)(const void *, const void *))a_comp);
		break;
	case NOCASE_LIST:
		qsort((void *)s, n, sizeof(char *),
			(int (*)(const void *, const void *))c_comp);
		break;
	case NUMERIC_LIST:
		ip = (i_rec *) XCALLOC(n, sizeof(i_rec), "do_asort");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = atoi(s[i]);
		}
		qsort((void *)ip, n, sizeof(i_rec), 
			(int (*)(const void *, const void *))i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		XFREE(ip, "do_asort");
		break;
	case DBREF_LIST:
		ip = (i_rec *) XCALLOC(n, sizeof(i_rec), "do_asort.2");
		for (i = 0; i < n; i++) {
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
		}
		qsort((void *)ip, n, sizeof(i_rec),
			(int (*)(const void *, const void *))i_comp);
		for (i = 0; i < n; i++) {
			s[i] = ip[i].str;
		}
		XFREE(ip, "do_asort.2");
		break;
	case FLOAT_LIST:
		fp = (f_rec *) XCALLOC(n, sizeof(f_rec), "do_asort.3");
		for (i = 0; i < n; i++) {
			fp[i].str = s[i];
			fp[i].data = aton(s[i]);
		}
		qsort((void *)fp, n, sizeof(f_rec),
			(int (*)(const void *, const void *))f_comp);
		for (i = 0; i < n; i++) {
			s[i] = fp[i].str;
		}
		XFREE(fp, "do_asort.3");
		break;
	}
}

FUNCTION(fun_sort)
{
	int nitems, sort_type, osep_len;
	char *list;
	Delim isep, osep;
	char *ptrs[LBUF_SIZE / 2];

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}

	VaChk_In_Out("SORT", 1, 4);

	/* Convert the list to an array */

	list = alloc_lbuf("fun_sort");
	strcpy(list, fargs[0]);
	nitems = list2arr(ptrs, LBUF_SIZE / 2, list, isep.c);
	sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
	do_asort(ptrs, nitems, sort_type);
	arr2list(ptrs, nitems, buff, bufc, osep, osep_len);
	free_lbuf(list);
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
	    ((mudconf.func_cpu_lim > 0) &&
	     (clock() - mudstate.cputime_base > mudconf.func_cpu_lim)))
		return 0;

	tbuf = alloc_lbuf("u_comp");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	strcpy(tbuf, ucomp_buff);
	result = bp = alloc_lbuf("u_comp");
	str = tbuf;
	exec(result, &bp, ucomp_player, ucomp_caller, ucomp_cause,
	     EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(elems[0]), 2);
	*bp = '\0';
	if (!result)
		n = 0;
	else {
		n = atoi(result);
		free_lbuf(result);
	}
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

	i = random() % (1 + (right - left));
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
	char *atext, *list, *ptrs[LBUF_SIZE / 2];
	Delim isep, osep;
	int nptrs, aflags, alen, anum, osep_len;
	dbref thing, aowner;
	ATTR *ap;

	if ((nfargs == 0) || !fargs[0] || !*fargs[0]) {
		return;
	}
	VaChk_Only_In_Out("SORTBY", 4);

	Parse_Uattr(player, fargs[0], thing, anum, ap);
	Get_Uattr(player, thing, ap, atext, aowner, aflags, alen);

	strcpy(ucomp_buff, atext);
	ucomp_player = thing;
	ucomp_caller = player;
	ucomp_cause = cause;

	list = alloc_lbuf("fun_sortby");
	strcpy(list, fargs[1]);
	nptrs = list2arr(ptrs, LBUF_SIZE / 2, list, isep.c);

	if (nptrs > 1)		/* pointless to sort less than 2 elements */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp);

	arr2list(ptrs, nptrs, buff, bufc, osep, osep_len);
	free_lbuf(list);
	free_lbuf(atext);
}

/* ---------------------------------------------------------------------------
 * fun_setunion, fun_setdiff, fun_setinter: Set management.
 * Also fun_lunion, fun_ldiff, fun_linter: Same thing, but takes
 * a sort type like sort() does. There's an unavoidable PennMUSH conflict,
 * as setunion() and friends have a 4th-arg output delimiter in TM3, but
 * the 4th arg is used for the sort type in PennMUSH. Also, adding the
 * sort type as a fifth arg for setunion(), etc. would be confusing, since
 * the last two args are, by convention, delimiters. So we add new funcs.
 */

#define	SET_UNION	1
#define	SET_INTERSECT	2
#define	SET_DIFF	3

#define NUMCMP(f1,f2) \
	((f1 > f2) ? 1 : ((f1 < f2) ? -1 : 0))

#define GENCMP(x1,x2,s) \
((s == ALPHANUM_LIST) ? strcmp(ptrs1[x1],ptrs2[x2]) : \
 ((s == NOCASE_LIST) ? strcasecmp(ptrs1[x1],ptrs2[x2]) : \
  ((s == FLOAT_LIST) ? NUMCMP(fp1[x1],fp2[x2]) : NUMCMP(ip1[x1],ip2[x2]))))

static void handle_sets(fargs, nfargs, buff, bufc, oper, isep, osep,
			osep_len, type_pos)
char *fargs[], *buff, **bufc;
Delim isep, osep;
int nfargs, oper, osep_len, type_pos;
{
	char *list1, *list2, *oldp, *bb_p;
	char *ptrs1[LBUF_SIZE], *ptrs2[LBUF_SIZE];
	int i1, i2, n1, n2, val, sort_type;
	int *ip1, *ip2;
	double *fp1, *fp2;

	list1 = alloc_lbuf("fun_setunion.1");
	strcpy(list1, fargs[0]);
	n1 = list2arr(ptrs1, LBUF_SIZE, list1, isep.c);
	if (type_pos == -1)
	    sort_type = ALPHANUM_LIST;
	else
	    sort_type = get_list_type(fargs, nfargs, 3, ptrs1, n1);
	do_asort(ptrs1, n1, sort_type);

	list2 = alloc_lbuf("fun_setunion.2");
	strcpy(list2, fargs[1]);
	n2 = list2arr(ptrs2, LBUF_SIZE, list2, isep.c);
	do_asort(ptrs2, n2, sort_type);

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
				        print_sep(osep, osep_len, buff, bufc);
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
					print_sep(osep, osep_len, buff, bufc);
				}
				oldp = *bufc;
				safe_str(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}
		for (; i2 < n2; i2++) {
			if (strcmp(oldp, ptrs2[i2])) {
			        if (*bufc != bb_p) {
				    print_sep(osep, osep_len, buff, bufc);
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
					print_sep(osep, osep_len, buff, bufc);
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
					print_sep(osep, osep_len, buff, bufc);
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
			    print_sep(osep, osep_len, buff, bufc);
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
}

FUNCTION(fun_setunion)
{
	Delim isep, osep;
	int osep_len;

	VaChk_Only_In_Out("SETUNION", 4);
	handle_sets(fargs, nfargs, buff, bufc, SET_UNION,
		    isep, osep, osep_len, -1);
	return;
}

FUNCTION(fun_setdiff)
{
	Delim isep, osep;
	int osep_len;

	VaChk_Only_In_Out("SETDIFF", 4);
	handle_sets(fargs, nfargs, buff, bufc, SET_DIFF,
		    isep, osep, osep_len, -1);
	return;
}

FUNCTION(fun_setinter)
{
	Delim isep, osep;
	int osep_len;

	VaChk_Only_In_Out("SETINTER", 4);
	handle_sets(fargs, nfargs, buff, bufc, SET_INTERSECT,
		    isep, osep, osep_len, -1);
	return;
}

FUNCTION(fun_lunion)
{
	Delim isep, osep;
	int osep_len;

	VaChk_In_Out("LUNION", 2, 5);
	handle_sets(fargs, nfargs, buff, bufc, SET_UNION,
		    isep, osep, osep_len, 3);
	return;
}

FUNCTION(fun_ldiff)
{
	Delim isep, osep;
	int osep_len;

	VaChk_In_Out("LDIFF", 2, 5);
	handle_sets(fargs, nfargs, buff, bufc, SET_DIFF,
		    isep, osep, osep_len, 3);
	return;
}

FUNCTION(fun_linter)
{
	Delim isep, osep;
	int osep_len;

	VaChk_In_Out("LINTER", 2, 5);
	handle_sets(fargs, nfargs, buff, bufc, SET_INTERSECT,
		    isep, osep, osep_len, 3);
	return;
}

/*---------------------------------------------------------------------------
 * Format a list into columns.
 */

FUNCTION(fun_columns)
{
	unsigned int spaces, number, ansinumber, striplen;
	unsigned int count, i, indent = 0;
	int isansi = 0, rturn = 1, cr = 0;
	char *p, *q, *buf, *curr, *objstring, *bp, *cp, *str;
	Delim isep;

	VaChk_Range("COLUMNS", 2, 4);
	VaChk_InSep(3, DELIM_EVAL);
		
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

	cp = curr = bp = alloc_lbuf("fun_columns");
	str = fargs[0];
	exec(curr, &bp, player, caller, cause,
	     EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	*bp = '\0';
	cp = trim_space_sep(cp, isep.c);
	if (!*cp) {
		free_lbuf(curr);
		return;
	}
	
	for (i = 0; i < indent; i++)
		safe_chr(' ', buff, bufc);

	buf = alloc_lbuf("fun_columns");
	
	while (cp) {
		objstring = split_token(&cp, isep.c);
		ansinumber = number;
		striplen = strlen((char *) strip_ansi(objstring));
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
			cr = 1;
			for (i = 0; i < indent; i++)
				safe_chr(' ', buff, bufc);
		} else {
			cr = 0;
		}

		rturn++;
	}
	
	if (!cr) {
		safe_crlf(buff, bufc);
	}
	
	free_lbuf(buf);
	free_lbuf(curr);
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

#define TABLES_LJUST	0 
#define TABLES_RJUST	1
#define TABLES_CENTER	2

static void tables_helper(list, last_state, n_cols, col_widths,
			  lead_str, trail_str, list_sep, field_sep, pad_char,
			  buff, bufc, key)
    char *list, *last_state;
    int n_cols, col_widths[];
    char *lead_str, *trail_str, list_sep, *field_sep, pad_char;
    char *buff, **bufc;
    int key;
{
    int i, nwords, nstates, cpos, wcount, over, have_normal;
    int max, nleft, lead_chrs, lens[LBUF_SIZE / 2];
    char *s, *savep, *p, tbuf[LBUF_SIZE];
    char *words[LBUF_SIZE / 2], *states[LBUF_SIZE / 2];

    /* Split apart the list. We need to find the length of each de-ansified
     * word, as well as keep track of the state of each word.
     * Overly-long words eventually get truncated, but the correct ANSI
     * state is preserved nonetheless.
     */
    strcpy(tbuf, list);
    nstates = list2ansi(states, last_state, LBUF_SIZE / 2, tbuf, list_sep);
    nwords = list2arr(words, LBUF_SIZE / 2, list, list_sep);
    if (nstates != nwords) {
	for (i = 0; i < nstates; i++) {
	    XFREE(states[i], "list2ansi");
	}
	return;
    }
    for (i = 0; i < nwords; i++)
	lens[i] = strlen(strip_ansi(words[i]));

    over = wcount = 0;
    while ((wcount < nwords) && !over) {

	/* Beginning of new line. Insert newline if this isn't the first
	 * thing we're writing. Write left margin, if appropriate.
	 */
	if (wcount != 0)
	    safe_crlf(buff, bufc);

	if (lead_str)
	    over = safe_str(lead_str, buff, bufc);

	/* Do each column in the line. */

	for (cpos = 0; (cpos < n_cols) && (wcount < nwords) && !over;
	     cpos++, wcount++) {

	    /* Write leading padding if we need it. */

	    if (key == TABLES_RJUST) {
		nleft = col_widths[cpos] - lens[wcount];
		print_padding(nleft, max, pad_char);
	    } else if (key == TABLES_CENTER) {
		lead_chrs = (int)((col_widths[cpos] / 2) -
				  (lens[wcount] / 2) + .5);
		print_padding(lead_chrs, max, pad_char);
	    }

	    /* If we had a previous state, we have to write it. */

	    if ((wcount > 0) && *states[wcount - 1]) {
		print_ansi_state(p, states[wcount - 1]);
	    } else if ((wcount == 0) && *last_state) {
		print_ansi_state(p, last_state);
	    }

	    /* Copy in the word. */
	    
	    if (lens[wcount] <= col_widths[cpos]) {
		over = safe_str(words[wcount], buff, bufc);
		if (*states[wcount]) {
		    safe_ansi_normal(buff, bufc);
		}
	    } else {
		/* Bleah. We have a string that's too long. Truncate it.
		 * Write an ANSI normal at the end at the end if we need
		 * one (we'll restore the correct ANSI code with the
		 * next word, if need be).
		 */
		have_normal = 1;
		for (s = words[wcount], i = 0;
		     *s && (i < col_widths[cpos]); ) {
		    if (*s == ESC_CHAR) {
			Skip_Ansi_Code(s);
		    } else {
			safe_chr(*s, buff, bufc);
			s++;
			i++;
		    }
		}
		if (!have_normal || *states[wcount])
		    safe_ansi_normal(buff, bufc);
	    }

	    /* Writing trailing padding if we need it. */
	    
	    if (key == TABLES_LJUST) {
		nleft = col_widths[cpos] - lens[wcount];
		print_padding(nleft, max, pad_char);
	    } else if (key == TABLES_CENTER) {
		nleft = col_widths[cpos] - lead_chrs - lens[wcount];
		print_padding(nleft, max, pad_char);
	    }

	    /* Insert the field separator if this isn't the last column
	     * AND this is not the very last word in the list.
	     */

	    if ((cpos < n_cols - 1) && (wcount < nwords - 1))
		safe_str(field_sep, buff, bufc);
	}

	if (!over && trail_str) {

	    /* If we didn't get enough columns to fill out a line, and
	     * this is the last line, then we have to pad it out.
	     */
	    if ((wcount == nwords) &&
		((nleft = nwords % n_cols) > 0)) {
		for (cpos = nleft; (cpos < n_cols) && !over; cpos++) {
		    over = safe_str(field_sep, buff, bufc);
		    print_padding(col_widths[cpos], max, pad_char);
		}
	    }

	    /* Write the right margin. */

	    over = safe_str(trail_str, buff, bufc);
	}
    }

    /* Save the ANSI state of the last word. */

    strcpy(last_state, states[nstates - 1]);

    /* Clean up. */

    for (i = 0; i < nstates; i++) {
	XFREE(states[i], "list2ansi");
    }
}

static void perform_tables(player, list, n_cols, col_widths,
			   lead_str, trail_str, list_sep, field_sep, pad_char,
			   buff, bufc, key)
    dbref player;
    char *list;
    int n_cols, col_widths[];
    char *lead_str, *trail_str, list_sep, *field_sep, pad_char;
    char *buff, **bufc;
    int key;
{
    char *p, *savep, *bb_p, last_state[LBUF_SIZE];

    if (!list || !*list)
	return;

    if ((list_sep == ESC_CHAR) || (list_sep == ANSI_END)) {
	notify_quiet(player, "#-1 ILLEGAL LIST SEPARATOR");
	return;
    }

    bb_p = *bufc;
    savep = list;
    last_state[0] = '\0';
    p = strchr(list, '\r');
    while (p) {
	*p = '\0';
	if (*bufc != bb_p)
	    safe_crlf(buff, bufc);
	tables_helper(savep, last_state, n_cols, col_widths,
		      lead_str, trail_str, list_sep, field_sep, pad_char,
		      buff, bufc, key);
	savep = p + 2;	/* must skip '\n' too */
	p = strchr(savep, '\r'); 
    }
    if (*bufc != bb_p)
	safe_crlf(buff, bufc);
    tables_helper(savep, last_state, n_cols, col_widths, lead_str, trail_str,
		  list_sep, field_sep, pad_char, buff, bufc, key);
}

static void process_tables(buff, bufc, player, caller, cause, fargs, nfargs,
			   cargs, ncargs, key)
    char *buff, **bufc;
    dbref player, caller, cause;
    char *fargs[], *cargs[];
    int nfargs, ncargs, key;
{
    int i, num, n_columns, *col_widths;
    Delim list_sep, pad_char;
    char fs_buf[2], *widths[LBUF_SIZE / 2];  
    char **fseps = NULL;

    if (!delim_check(fargs, nfargs, 5, &list_sep, buff, bufc,
		     player, caller, cause, cargs, ncargs, 0))
	return;
    if (!delim_check(fargs, nfargs, 7, &pad_char, buff, bufc,
		     player, caller, cause, cargs, ncargs, 0))
	return;

    /* Handle the field separator. */

    if ((nfargs < 6) || !*fargs[5]) {
	fs_buf[0] = ' ';
	fs_buf[1] = '\0';
    }

    n_columns = list2arr(widths, LBUF_SIZE / 2, fargs[1], ' ');
    col_widths = (int *) XCALLOC(n_columns, sizeof(int), "fun_table.widths");
    for (i = 0; i < n_columns; i++) {
	num = atoi(widths[i]);
	col_widths[i] = (num < 1) ? 1 : num;
    }

    perform_tables(player, fargs[0], n_columns, col_widths,
		   ((nfargs > 2) && *fargs[2]) ? fargs[2] : NULL,
		   ((nfargs > 3) && *fargs[3]) ? fargs[3] : NULL, 
		   list_sep.c,
		   ((nfargs > 5) && *fargs[5]) ? fargs[5] : fs_buf,
		   pad_char.c, buff, bufc, key);

    XFREE(col_widths, "fun_table.widths");
}

FUNCTION(fun_tables)
{
    VaChk_Range("TABLES", 2, 7);
    process_tables(buff, bufc, player, caller, cause, fargs, nfargs,
		   cargs, ncargs, TABLES_LJUST);
}

FUNCTION(fun_rtables)
{
    VaChk_Range("RTABLES", 2, 7);
    process_tables(buff, bufc, player, caller, cause, fargs, nfargs,
		   cargs, ncargs, TABLES_RJUST);
}

FUNCTION(fun_ctables)
{
    VaChk_Range("CTABLES", 2, 7);
    process_tables(buff, bufc, player, caller, cause, fargs, nfargs,
		   cargs, ncargs, TABLES_CENTER);
}

FUNCTION(fun_table)
{
    int line_length = 78;
    int field_width = 10;
    int i, n_columns, *col_widths;
    Delim list_sep, field_sep, pad_char;
    char fs_buf[2];

    VaChk_Range("TABLE", 1, 6);
    if (!delim_check(fargs, nfargs, 4, &list_sep, buff, bufc,
		     player, caller, cause, cargs, ncargs, 0))
	return;
    if (!delim_check(fargs, nfargs, 5, &field_sep, buff, bufc,
		     player, caller, cause, cargs, ncargs, 0))
	return;
    if (!delim_check(fargs, nfargs, 6, &pad_char, buff, bufc,
		     player, caller, cause, cargs, ncargs, 0))
	return;

    /* Get line length and column width. All columns are the same width.
     * Calculate what we need to.
     */

    if (nfargs > 2) {
	line_length = atoi(fargs[2]);
	if (line_length < 2)
	    line_length = 2;
    }

    if (nfargs > 1) {
	field_width = atoi(fargs[1]);
	if (field_width < 1)
	    field_width = 1;
	else if (field_width > LBUF_SIZE - 1)
	    field_width = LBUF_SIZE - 1;
    }

    if (field_width >= line_length)
	field_width = line_length - 1;

    n_columns = (int)(line_length / (field_width + 1));
    col_widths = (int *) XCALLOC(n_columns, sizeof(int), "fun_table.widths");
    for (i = 0; i < n_columns; i++)
	col_widths[i] = field_width;

    fs_buf[0] = field_sep.c;
    fs_buf[1] = '\0'; 
    perform_tables(player, fargs[0], n_columns, col_widths, NULL, NULL,
		   list_sep.c, fs_buf, pad_char.c, buff, bufc, TABLES_LJUST);

    XFREE(col_widths, "fun_table.widths");
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
	int nwords, cur, osep_len;
	char *ptrs[LBUF_SIZE / 2];
	char *wordlist, *s, *r, *oldp;
	Delim isep, osep;

	VaChk_Only_In_Out("ELEMENTS", 4);
	oldp = *bufc;

	/* Turn the first list into an array. */

	wordlist = alloc_lbuf("fun_elements.wordlist");
	strcpy(wordlist, fargs[0]);
	nwords = list2arr(ptrs, LBUF_SIZE / 2, wordlist, isep.c);

	s = trim_space_sep(fargs[1], ' ');

	/* Go through the second list, grabbing the numbers and finding the
	 * corresponding elements. 
	 */

	do {
		r = split_token(&s, ' ');
		cur = atoi(r) - 1;
		if ((cur >= 0) && (cur < nwords) && ptrs[cur]) {
		    if (oldp != *bufc) {
			print_sep(osep, osep_len, buff, bufc);
		    }
		    safe_str(ptrs[cur], buff, bufc);
		}
	} while (s);
	free_lbuf(wordlist);
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

	VaChk_Only_In("GRAB", 3);

	/* Walk the wordstring, until we find the word we want. */

	s = trim_space_sep(fargs[0], isep.c);
	do {
		r = split_token(&s, isep.c);
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
    int osep_len;

    VaChk_Only_In_Out("GRABALL", 4);

    s = trim_space_sep(fargs[0], isep.c);
    bb_p = *bufc;
    do {
	r = split_token(&s, isep.c);
	if (quick_wild(fargs[1], r)) {
	    if (*bufc != bb_p) {
		print_sep(osep, osep_len, buff, bufc);
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
	char *words[LBUF_SIZE];
	int n, i, j, osep_len;
	Delim isep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0]) {
		return;
	}
	VaChk_Only_In_Out("SHUFFLE", 3);

	n = list2arr(words, LBUF_SIZE, fargs[0], isep.c);

	for (i = 0; i < n; i++) {
		j = (random() % (n - i)) + i;
		swap(&words[i], &words[j]);
	}
	arr2list(words, n, buff, bufc, osep, osep_len);
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
    int nptrs_old, nptrs_new, osep_len;
    char *ptrs_old[LBUF_SIZE / 2], *ptrs_new[LBUF_SIZE / 2];
    char *r, *s, *bb_p;
    int i;
    int got_it;

    VaChk_Only_In_Out("LEDIT", 5);

    old_list = alloc_lbuf("fun_ledit.old");
    new_list = alloc_lbuf("fun_ledit.new");
    strcpy(old_list, fargs[1]);
    strcpy(new_list, fargs[2]);
    nptrs_old = list2arr(ptrs_old, LBUF_SIZE / 2, old_list, isep.c);
    nptrs_new = list2arr(ptrs_new, LBUF_SIZE / 2, new_list, isep.c);

    /* Iterate through the words. */

    bb_p = *bufc;
    s = trim_space_sep(fargs[0], isep.c);
    do {
	if (*bufc != bb_p) {
	    print_sep(osep, osep_len, buff, bufc);
	}
	r = split_token(&s, isep.c);
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
}
