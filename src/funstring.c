/* funstring.c - string functions */
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
#include "powers.h"	/* required by code */
#include "ansi.h"	/* required by code */

/* ---------------------------------------------------------------------------
 * isword: is every character in the argument a letter?
 * isalnum: is every character in the argument a letter or number?
 */
  
FUNCTION(fun_isword)
{
char *p;
      
	for (p = fargs[0]; *p; p++) {
		if (!isalpha(*p)) {
			safe_chr('0', buff, bufc);
			return;
		}
	}
	safe_chr('1', buff, bufc);
}

FUNCTION(fun_isalnum)
{
     char *p;

     for (p = fargs[0]; *p; p++) {
	 if (!isalnum(*p)) {
	     safe_chr('0', buff, bufc); return;
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
 * fun_null: Just eat the contents of the string. Handy for those times
 *           when you've output a bunch of junk in a function call and
 *           just want to dispose of the output (like if you've done an
 *           iter() that just did a bunch of side-effects, and now you have
 *           bunches of spaces you need to get rid of.
 */

FUNCTION(fun_null)
{
	return;
}

/* ---------------------------------------------------------------------------
 * fun_squish: Squash occurrences of a given character down to 1.
 *             We do this both on leading and trailing chars, as well as
 *             internal ones; if the player wants to trim off the leading
 *             and trailing as well, they can always call trim().
 */

FUNCTION(fun_squish)
{
	char *tp, *bp;
	Delim isep;

	if (nfargs == 0) {
		return;
	}

	VaChk_Only_InPure(2);

	bp = tp = fargs[0];

	while (*tp) {

		/* Move over and copy the non-sep characters */

		while (*tp && *tp != isep.str[0]) {
			if (*tp == ESC_CHAR) {
				copy_esccode(tp, bp);
			} else {
				*bp++ = *tp++;
			}
		}

		/* If we've reached the end of the string, leave the loop. */

		if (!*tp)
			break;

		/* Otherwise, we've hit a sep char. Move over it, and then
		 * move on to the next non-separator. Note that we're
		 * overwriting our own string as we do this. However, the
		 * other pointer will always be ahead of our current copy
		 * pointer.
		 */

		*bp++ = *tp++;
		while (*tp && (*tp == isep.str[0]))
			tp++;
	}

	/* Must terminate the string */

	*bp = '\0';
    
	safe_str(fargs[0], buff, bufc);
}

/* ---------------------------------------------------------------------------
 * trim: trim off unwanted white space.
 */
#define TRIM_L 0x1
#define TRIM_R 0x2

FUNCTION(fun_trim)
{
	char *p, *q, *endchar, *ep;
	Delim isep;
	int trim;

	if (nfargs == 0) {
		return;
	}
	VaChk_In(1, 3);
	if (nfargs >= 2) {
		switch (tolower(*fargs[1])) {
		case 'l':
			trim = TRIM_L;
			break;
		case 'r':
			trim = TRIM_R;
			break;
		default:
			trim = TRIM_L | TRIM_R;
			break;
		}
	} else {
		trim = TRIM_L | TRIM_R;
	}

	p = fargs[0];

	/* Single-character delimiters are easy. */

	if (isep.len == 1) { 
	    if (trim & TRIM_L) {
		while (*p == isep.str[0]) {
			p++;
		}
	    }
	    if (trim & TRIM_R) {
		q = endchar = p;
		while (*q != '\0') {
			if (*q == ESC_CHAR) {
				skip_esccode(q);
				endchar = q;
			} else if (*q++ != isep.str[0]) {
				endchar = q;
			}
		}
		*endchar = '\0';
	    }
	    safe_str(p, buff, bufc);
	    return;
	}

	/* Multi-character delimiters take more work. */

	ep = p + strlen(fargs[0]) - 1; /* last char in string */

	if (trim & TRIM_L) {
	    while (!strncmp(p, isep.str, isep.len) && (p <= ep))
		p = p + isep.len;
	    if (p > ep)
		return;
	}

	if (trim & TRIM_R) {
	    q = endchar = p;
	    while (q <= ep) {
		if (*q == ESC_CHAR) {
		    skip_esccode(q);
		    endchar = q;
		} else if (!strncmp(q, isep.str, isep.len)) {
		    q = q + isep.len;
		} else {
		    q++;
		    endchar = q;
		}
	    }
	    *endchar = '\0';
	}

	safe_str(p, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified string.
 */

FUNCTION(fun_after)
{
	char *bp, *cp, *mp, *np;
	int ansi_needle, ansi_needle2, ansi_haystack, ansi_haystack2;

	if (nfargs == 0) {
		return;
	}
	VaChk_Range(1, 2);
	bp = fargs[0];	/* haystack */
	mp = fargs[1];	/* needle */

	/* Sanity-check arg1 and arg2 */

	if (bp == NULL)
		bp = "";
	if (mp == NULL)
		mp = " ";
	if (!mp || !*mp)
		mp = (char *)" ";
	if ((mp[0] == ' ') && (mp[1] == '\0'))
		bp = Eat_Spaces(bp);

	/* Get ansi state of the first needle char */
	ansi_needle = ANST_NONE;
	while (*mp == ESC_CHAR) {
		track_esccode(mp, ansi_needle);
		if (!*mp)
			mp = (char *)" ";
	}
	ansi_haystack = ANST_NORMAL;

	/* Look for the needle string */

	while (*bp) {
		while (*bp == ESC_CHAR) {
			track_esccode(bp, ansi_haystack);
		}

		if ((*bp == *mp) &&
		    (ansi_needle == ANST_NONE ||
		     ansi_haystack == ansi_needle)) {

			/* See if what follows is what we are looking for */
			ansi_needle2 = ansi_needle;
			ansi_haystack2 = ansi_haystack;

			cp = bp;
			np = mp;

			while (1) {
				while (*cp == ESC_CHAR) {
					track_esccode(cp, ansi_haystack2);
				}
				while (*np == ESC_CHAR) {
					track_esccode(np, ansi_needle2);
				}
				if ((*cp != *np) ||
				    (ansi_needle2 != ANST_NONE &&
				     ansi_haystack2 != ansi_needle2) ||
				    !*cp || !*np)
					break;
				++cp, ++np;
			}

			if (!*np) {
				/* Yup, return what follows */
				safe_str(ansi_transition_esccode(ANST_NORMAL, ansi_haystack2), buff, bufc);
				safe_str(cp, buff, bufc);
				return;
			}
		}

		/* Nope, continue searching */
		if (*bp)
			++bp;
	}

	/* Ran off the end without finding it */
	return;
}

FUNCTION(fun_before)
{
	char *haystack, *bp, *cp, *mp, *np;
	int ansi_needle, ansi_needle2, ansi_haystack, ansi_haystack2;

	if (nfargs == 0) {
		return;
	}
	VaChk_Range(1, 2);
	haystack = fargs[0];	/* haystack */
	mp = fargs[1];		/* needle */

	/* Sanity-check arg1 and arg2 */

	if (haystack == NULL)
		haystack = "";
	if (mp == NULL)
		mp = " ";
	if (!mp || !*mp)
		mp = (char *)" ";
	if ((mp[0] == ' ') && (mp[1] == '\0'))
		haystack = Eat_Spaces(haystack);
	bp = haystack;

	/* Get ansi state of the first needle char */
	ansi_needle = ANST_NONE;
	while (*mp == ESC_CHAR) {
		track_esccode(mp, ansi_needle);
		if (!*mp)
			mp = (char *)" ";
	}
	ansi_haystack = ANST_NORMAL;

	/* Look for the needle string */

	while (*bp) {
		/* See if what follows is what we are looking for */
		ansi_needle2 = ansi_needle;
		ansi_haystack2 = ansi_haystack;

		cp = bp;
		np = mp;

		while (1) {
			while (*cp == ESC_CHAR) {
				track_esccode(cp, ansi_haystack2);
			}
			while (*np == ESC_CHAR) {
				track_esccode(np, ansi_needle2);
			}
			if ((*cp != *np) ||
			    (ansi_needle2 != ANST_NONE &&
			     ansi_haystack2 != ansi_needle2) ||
			    !*cp || !*np)
				break;
			++cp, ++np;
		}

		if (!*np) {
			/* Yup, return what came before this */
			*bp = '\0';
			safe_str(haystack, buff, bufc);
			safe_str(ansi_transition_esccode(ansi_haystack, ANST_NORMAL), buff, bufc);
			return;
		}

		/* Nope, continue searching */
		while (*bp == ESC_CHAR) {
			track_esccode(bp, ansi_haystack);
		}

		if (*bp)
			++bp;
	}

	/* Ran off the end without finding it */
	safe_str(haystack, buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

FUNCTION(fun_lcstr)
{
	char *ap;

	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap) {
		if (*ap == ESC_CHAR) {
			skip_esccode(ap);
		} else {
			*ap = tolower(*ap);
			ap++;
		}
	}
}

FUNCTION(fun_ucstr)
{
	char *ap;

	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap) {
		if (*ap == ESC_CHAR) {
			skip_esccode(ap);
		} else {
			*ap = toupper(*ap);
			ap++;
		}
	}
}

FUNCTION(fun_capstr)
{
	char *ap;

	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap == ESC_CHAR) {
		skip_esccode(ap);
	}

	*ap = toupper(*ap);
}

/* ---------------------------------------------------------------------------
 * fun_space: Make spaces.
 */

FUNCTION(fun_space)
{
	int num, max;

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
	memset(*bufc, ' ', num);
	*bufc += num;
	**bufc = '\0';
}

/* ---------------------------------------------------------------------------
 * rjust, ljust, center: Justify or center text, specifying fill character
 */

FUNCTION(fun_ljust)
{
	int spaces, max, i, slen;
	char *tp, *fillchars;

	VaChk_Range(2, 3);
	spaces = atoi(fargs[1]) - strip_ansi_len(fargs[0]);

	safe_str(fargs[0], buff, bufc);

	/* Sanitize number of spaces */
	if (spaces <= 0)
		return;	/* no padding needed, just return string */

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2]) {
		fillchars = strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;
		if (slen == 0) {
			/* NULL character fill */
			memset(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1) {
			/* single character fill */
			memset(tp, *fillchars, spaces);
			tp += spaces;
		}
		else {
			/* multi character fill */
			for (i = spaces; i >= slen; i -= slen) {
				memcpy(tp, fillchars, slen);
				tp += slen;
			}
			if (i) {
				/* we have a remainder here */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else {
		/* no fill character specified */
		memset(tp, ' ', spaces);
		tp += spaces;
	}

	*tp = '\0';
	*bufc = tp;
}

FUNCTION(fun_rjust)
{
	int spaces, max, i, slen;
	char *tp, *fillchars;

	VaChk_Range(2, 3);
	spaces = atoi(fargs[1]) - strip_ansi_len(fargs[0]);

	/* Sanitize number of spaces */

	if (spaces <= 0) {
		/* no padding needed, just return string */
		safe_str(fargs[0], buff, bufc);
		return;
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2]) {
		fillchars = strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;
		if (slen == 0) {
			/* NULL character fill */
			memset(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1) {
			/* single character fill */
			memset(tp, *fillchars, spaces);
			tp += spaces;
		}
		else {
			/* multi character fill */
			for (i = spaces; i >= slen; i -= slen) {
				memcpy(tp, fillchars, slen);
				tp += slen;
			}
			if (i) {
				/* we have a remainder here */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else {
		/* no fill character specified */
		memset(tp, ' ', spaces);
		tp += spaces;
	}

	*bufc = tp;
	safe_str(fargs[0], buff, bufc);
}

FUNCTION(fun_center)
{
	char *tp, *fillchars;
	int len, lead_chrs, trail_chrs, width, max, i, slen;

	VaChk_Range(2, 3);

	width = atoi(fargs[1]);
	len = strip_ansi_len(fargs[0]);

	width = (width > LBUF_SIZE - 1) ? LBUF_SIZE - 1 : width;
	if (len >= width) {
		safe_str(fargs[0], buff, bufc);
		return;
	}

	lead_chrs = (int)((width / 2) - (len / 2) + .5);
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	lead_chrs = (lead_chrs > max) ? max : lead_chrs;

	if (fargs[2]) {
		fillchars = (char *)strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > lead_chrs) ? lead_chrs : slen;
		if (slen == 0) {
			/* NULL character fill */
			memset(tp, ' ', lead_chrs);
			tp += lead_chrs;
		}
		else if (slen == 1) {
			/* single character fill */
			memset(tp, *fillchars, lead_chrs);
			tp += lead_chrs;
		}
		else {
			/* multi character fill */
			for (i = lead_chrs; i >= slen; i -= slen) {
				memcpy(tp, fillchars, slen);
				tp += slen;
			}
			if (i) {
				/* we have a remainder here */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else {
		/* no fill character specified */
		memset(tp, ' ', lead_chrs);
		tp += lead_chrs;
	}

	*bufc = tp;

	safe_str(fargs[0], buff, bufc);

	trail_chrs = width - lead_chrs - len;
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff);
	trail_chrs = (trail_chrs > max) ? max : trail_chrs;

	if (fargs[2]) {
		if (slen == 0) {
			/* NULL character fill */
			memset(tp, ' ', trail_chrs);
			tp += trail_chrs;
		}
		else if (slen == 1) {
			/* single character fill */
			memset(tp, *fillchars, trail_chrs);
			tp += trail_chrs;
		}
		else {
			/* multi character fill */
			for (i = trail_chrs; i >= slen; i -= slen) {
				memcpy(tp, fillchars, slen);
				tp += slen;
			}
			if (i) {
				/* we have a remainder here */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else {
		/* no fill character specified */
		memset(tp, ' ', trail_chrs);
		tp += trail_chrs;
	}

	*tp = '\0';
	*bufc = tp;
}

/* ---------------------------------------------------------------------------
 * fun_left: Returns first n characters in a string
 * fun_right: Returns last n characters in a string
 * strtrunc: now an alias for left
 */

FUNCTION(fun_left)
{
    char *s;
    int count, nchars;
    int ansi_state = ANST_NORMAL;

    s = fargs[0];
    nchars = atoi(fargs[1]);

    if (nchars <= 0)
	return;

    for (count = 0; (count < nchars) && *s; count++) {
	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state);
	}

	if (*s) {
	    ++s;
	}
    }

    safe_known_str(fargs[0], s - fargs[0], buff, bufc);
    safe_str(ansi_transition_esccode(ansi_state, ANST_NORMAL), buff, bufc);
}

FUNCTION(fun_right)
{
    char *s;
    int count, start, nchars;
    int ansi_state = ANST_NORMAL;

    s = fargs[0];
    nchars = atoi(fargs[1]);
    start = strip_ansi_len(s) - nchars;

    if (nchars <= 0)
	return;

    if (start < 0) {
	nchars += start;
	if (nchars <= 0)
	    return;
	start = 0;
    }

    while (*s == ESC_CHAR) {
	track_esccode(s, ansi_state);
    }

    for (count = 0; (count < start) && *s; count++) {
	++s;

	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state);
	}
    }

    if (*s) {
	safe_str(ansi_transition_esccode(ANST_NORMAL, ansi_state), buff, bufc);
    }

    safe_str(s, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_chomp: If the line ends with CRLF, CR, or LF, chop it off.
 */

FUNCTION(fun_chomp)
{
	char *bb_p = *bufc;

	safe_str(fargs[0], buff, bufc);
	if (*bufc != bb_p && (*bufc)[-1] == '\n')
		(*bufc)--;
	if (*bufc != bb_p && (*bufc)[-1] == '\r')
		(*bufc)--;
}

/* ---------------------------------------------------------------------------
 * fun_comp: exact-string compare.
 * fun_streq: non-case-sensitive string compare.
 * fun_strmatch: wildcard string compare.
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

FUNCTION(fun_streq)
{
    safe_bool(buff, bufc, !string_compare(fargs[0], fargs[1]));
}

FUNCTION(fun_strmatch)
{
	/* Check if we match the whole string.  If so, return 1 */

	safe_bool(buff, bufc, quick_wild(fargs[1], fargs[0]));
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
 * fun_secure, fun_escape: escape [, ], %, \, and the beginning of the string.
 */

FUNCTION(fun_secure)
{
	char *s;

	s = fargs[0];
	while (*s) {
		switch (*s) {
		case ESC_CHAR:
			safe_copy_esccode(s, buff, bufc);
			continue;
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
		++s;
	}
}

FUNCTION(fun_escape)
{
	char *s, *d;

	s = fargs[0];
	if (!*s)
		return;

	safe_chr('\\', buff, bufc);
	d = *bufc;

	while (*s) {
		switch (*s) {
		case ESC_CHAR:
			safe_copy_esccode(s, buff, bufc);
			continue;
		case '%':
		case '\\':
		case '[':
		case ']':
		case '{':
		case '}':
		case ';':
			if (*bufc != d)
				safe_chr('\\', buff, bufc);
			/* FALLTHRU */
		default:
			safe_chr(*s, buff, bufc);
		}
		++s;
	}
}

/* ---------------------------------------------------------------------------
 * ANSI handlers.
 */

FUNCTION(fun_ansi)
{
	char *s;
	int ansi_state;

	if (!mudconf.ansi_colors) {
		safe_str(fargs[1], buff, bufc);
		return;
	}

	if (!fargs[0] || !*fargs[0]) {
		safe_str(fargs[1], buff, bufc);
		return;
	}

	track_ansi_letters(s, fargs[0], ansi_state);

	safe_str(ansi_transition_esccode(ANST_NONE, ansi_state), buff, bufc);

	s = fargs[1];
	while (*s) {
		if (*s == ESC_CHAR) {
			track_esccode(s, ansi_state);
		} else {
			++s;
		}
	}
	safe_str(fargs[1], buff, bufc);

	safe_str(ansi_transition_esccode(ansi_state, ANST_NONE), buff, bufc);
}

FUNCTION(fun_stripansi)
{
	safe_str((char *)strip_ansi(fargs[0]), buff, bufc);
}

/*---------------------------------------------------------------------------
 * encrypt() and decrypt(): From DarkZone.
 */

/*
 * Copy over only alphanumeric chars 
 */
#define CRYPTCODE_LO   32	/* space */
#define CRYPTCODE_HI  126	/* tilde */
#define CRYPTCODE_MOD  95	/* count of printable ascii chars */

static void crunch_code(code)
char *code;
{
	char *in, *out;
	
	in = out = code;
	while (*in) {
		if ((*in >= CRYPTCODE_LO) && (*in <= CRYPTCODE_HI)) {
			*out++ = *in++;
		} else if (*in == ESC_CHAR) {
			skip_esccode(in);
		} else {
			++in;
		}
	}
	*out = '\0';
}

static void crypt_code(buff, bufc, code, text, type)
char *buff, **bufc, *code, *text;
int type;
{
	char *p, *q;

	if (!*text)
		return;
	crunch_code(code);
	if (!*code) {
		safe_str(text, buff, bufc);
		return;
	}

	q = code;

	p = *bufc;
	safe_str(text, buff, bufc);

	/*
	 * Encryption: Simply go through each character of the text, get its
	 * ascii value, subtract LO, add the ascii value (less
	 * LO) of the code, mod the result, add LO. Continue
	 */
	while (*p) {
		if ((*p >= CRYPTCODE_LO) && (*p <= CRYPTCODE_HI)) {
			if (type) {
				*p = (((*p - CRYPTCODE_LO) + (*q - CRYPTCODE_LO)) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			} else {
				*p = (((*p - *q) + 2 * CRYPTCODE_MOD) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			}
			++p, ++q;

			if (!*q) {
				q = code;
			}
		} else if (*p == ESC_CHAR) {
			skip_esccode(p);
		} else {
			++p;
		}
	}
}

FUNCTION(fun_encrypt)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 1);
}

FUNCTION(fun_decrypt)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 0);
}

/* ---------------------------------------------------------------------------
 * fun_scramble:  randomizes the letters in a string.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_scramble)
{
	int n, i, j, ansi_state, *ansi_map;
	char *stripped;

	if (!fargs[0] || !*fargs[0]) {
	    return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);

	ansi_state = ANST_NORMAL;

	for (i = 0; i < n; i++) {
		j = random_range(i, n - 1);

		if (ansi_state != ansi_map[j]) {
			safe_str(ansi_transition_esccode(ansi_state,
							 ansi_map[j]),
				 buff, bufc);
			ansi_state = ansi_map[j];
		}

		safe_chr(stripped[j], buff, bufc);
		ansi_map[j] = ansi_map[i];
		stripped[j] = stripped[i];
	}

	safe_str(ansi_transition_esccode(ansi_state, ANST_NORMAL),
		 buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_reverse: reverse a string
 */

FUNCTION(fun_reverse)
{
	int n, *ansi_map, ansi_state;
	char *stripped;

	if (!fargs[0] || !*fargs[0]) {
	    return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);

	ansi_state = ansi_map[n];

	while (n--) {
		if (ansi_state != ansi_map[n]) {
			safe_str(ansi_transition_esccode(ansi_state,
							 ansi_map[n]),
				 buff, bufc);
			ansi_state = ansi_map[n];
		}
		safe_chr(stripped[n], buff, bufc);
	}

	safe_str(ansi_transition_esccode(ansi_state, ANST_NORMAL),
		 buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

FUNCTION(fun_mid)
{
    char *s, *savep;
    int count, start, nchars;
    int ansi_state = ANST_NORMAL;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);

    if (nchars <= 0)
	return;

    if (start < 0) {
	nchars += start;
	if (nchars <= 0)
	    return;
	start = 0;
    }

    while (*s == ESC_CHAR) {
	track_esccode(s, ansi_state);
    }

    for (count = 0; (count < start) && *s; ++count) {
	++s;

	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state);
	}
    }

    if (*s) {
	safe_str(ansi_transition_esccode(ANST_NORMAL, ansi_state), buff, bufc);
    }

    savep = s;
    for (count = 0; (count < nchars) && *s; ++count) {
	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state);
	}

	if (*s) {
	    ++s;
	}
    }

    safe_known_str(savep, s - savep, buff, bufc);

    safe_str(ansi_transition_esccode(ansi_state, ANST_NORMAL), buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second argument
 * is 0 or s, control characters are converted to spaces. If it's 1 or p,
 * they're converted to percent substitutions.
 */

FUNCTION(fun_translate)
{
	VaChk_Range(1, 2);

	/* Strictly speaking, we're just checking the first char */

	if (nfargs > 1 && (fargs[1][0] == 's' || fargs[1][0] == '0'))
		safe_str(translate_string(fargs[0], 0), buff, bufc);
	else if (nfargs > 1 && fargs[1][0] == 'p')
		safe_str(translate_string(fargs[0], 1), buff, bufc);
	else
		safe_str(translate_string(fargs[0], 1), buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_pos: Find a word in a string 
 */

FUNCTION(fun_pos)
{
	int i = 1;
	char *b, *s, *t, *u;
	char tbuf[LBUF_SIZE];

	i = 1;
	strcpy(tbuf, strip_ansi(fargs[0])); /* copy from static buff */
	b = tbuf;
	s = strip_ansi(fargs[1]);

	if (*b && !*(b+1)) {	/* single character */
	    t = strchr(s, *b);
	    if (t) {
		safe_ltos(buff, bufc, (int) (t - s + 1));
	    } else {
		safe_nothing(buff, bufc);
	    }
	    return;
	}

	while (*s) {
		u = s;
		t = b;
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
    char *s, *bb_p, *scratch_chartab;
    int i;
    Delim osep; 

    if (!fargs[0] || !*fargs[0])
	return;

    VaChk_Only_Out(3);

    scratch_chartab = (char *) XCALLOC(256, sizeof(char), "lpos.chartab");

    if (!fargs[1] || !*fargs[1]) {
	scratch_chartab[(unsigned char) ' '] = 1;
    } else {
	for (s = fargs[1]; *s; s++)
	    scratch_chartab[(unsigned char) *s] = 1;
    }

    bb_p = *bufc;

    for (i = 0, s = strip_ansi(fargs[0]); *s; i++, s++) {
	if (scratch_chartab[(unsigned char) *s]) {
	    if (*bufc != bb_p) {
		print_sep(&osep, buff, bufc);
	    }
	    safe_ltos(buff, bufc, i);
	}
    }

    XFREE(scratch_chartab, "lpos.chartab");
}

/* ---------------------------------------------------------------------------
 * Take a character position and return which word that char is in.
 * wordpos(<string>, <charpos>)
 */

FUNCTION(fun_wordpos)
{
	int charpos, i;
	char *cp, *tp, *xp;
	Delim isep;

	VaChk_Only_In(3);

	charpos = atoi(fargs[1]);
	cp = strip_ansi(fargs[0]);
	if ((charpos > 0) && (charpos <= (int)strlen(cp))) {
		tp = &(cp[charpos - 1]);
		cp = trim_space_sep(cp, &isep);
		xp = split_token(&cp, &isep);
		for (i = 1; xp; i++) {
			if (tp < (xp + strlen(xp)))
				break;
			xp = split_token(&cp, &isep);
		}
		safe_ltos(buff, bufc, i);
		return;
	}
	safe_nothing(buff, bufc);
	return;
}

/* ---------------------------------------------------------------------------
 * Take a character position and return what color that character would be.
 * ansipos(<string>, <charpos>[, <type>])
 */

FUNCTION(fun_ansipos)
{
	int charpos, i, ansi_state;
	char *s;

	VaChk_Range(2, 3);

	s = fargs[0];
	charpos = atoi(fargs[1]);
	ansi_state = ANST_NORMAL;
	i = 0;

	while (*s && i < charpos) {
		if (*s == ESC_CHAR) {
			track_esccode(s, ansi_state);
		} else {
			++s, ++i;
		}
	}

	if (nfargs > 2 && (fargs[2][0] == 'e' || fargs[2][0] == '0'))
		safe_str(ansi_transition_esccode(ANST_NONE, ansi_state),
			 buff, bufc);
	else if (nfargs > 2 && (fargs[2][0] == 'p' || fargs[2][0] == '1'))
		safe_str(ansi_transition_mushcode(ANST_NONE, ansi_state),
			 buff, bufc);
	else
		safe_str(ansi_transition_letters(ANST_NONE, ansi_state),
			 buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

FUNCTION(fun_repeat)
{
    int times, len, i, maxtimes;

    times = atoi(fargs[1]);
    if ((times < 1) || (fargs[0] == NULL) || (!*fargs[0])) {
	return;
    } else if (times == 1) {
	safe_str(fargs[0], buff, bufc);
    } else {
	len = strlen(fargs[0]);
	maxtimes = (LBUF_SIZE - 1 - (*bufc - buff)) / len;
	maxtimes = (times > maxtimes) ? maxtimes : times;

	for (i = 0; i < maxtimes; i++) {
		memcpy(*bufc, fargs[0], len);
		*bufc += len;
	}
	if (times > maxtimes) {
		safe_known_str(fargs[0], len, buff, bufc);
	}
    }
}

/* ---------------------------------------------------------------------------
 * perform_border: Turn a string of words into a bordered paragraph:
 * BORDER, CBORDER, RBORDER
 * border(<words>,<width without margins>[,<L margin fill>[,<R margin fill>]])
 */

FUNCTION(perform_border)
{
    int width, just;
    char *l_fill, *r_fill, *bb_p;
    char *sl, *el, *sw, *ew;
    int sl_ansi_state, el_ansi_state, sw_ansi_state, ew_ansi_state;
    int sl_pos, el_pos, sw_pos, ew_pos;
    int nleft, max, lead_chrs;

    just = Func_Mask(JUST_TYPE);

    VaChk_Range(2, 4);

    if (!fargs[0] || !*fargs[0])
	return;

    width = atoi(fargs[1]);
    if (width < 1)
	width = 1;

    if (nfargs > 2) {
	l_fill = fargs[2];
    } else {
	l_fill = NULL;
    }
    if (nfargs > 3) {
	r_fill = fargs[3];
    } else {
	r_fill = NULL;
    }

    bb_p = *bufc;

    sl = el = sw = NULL;
    ew = fargs[0];
    sl_ansi_state = el_ansi_state = ANST_NORMAL;
    sw_ansi_state = ew_ansi_state = ANST_NORMAL;
    sl_pos = el_pos = sw_pos = ew_pos = 0;

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

      /* Three ways out of that loop: end-of-string (ES), end-of-line (EL),
       * and start-of-word (SW)
       */
      if (!*sw && sl == NULL) /* ES, and nothing left to output */
	break;

      /* Decide where start-of-line (SL) was */
      if (sl == NULL) {
	if (ew == fargs[0] || ew[-1] == '\n') {
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

	/* Three ways out of that loop: ES, EL, EW */

	/* If it fits on the line, add it */
	if (ew_pos - sl_pos <= width) {
	  el = ew;
	  el_ansi_state = ew_ansi_state;
	  el_pos = ew_pos;
	}

	/* If it's just EW, not ES or EL, and the line isn't too long,
	 * keep adding words to the line
	 */
	if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
	  continue;

	/* So now we definitely need to output a line */
      }

      /* Could be a blank line, no words fit */
      if (el == NULL) {
	el = sw;
	el_ansi_state = sw_ansi_state;
	el_pos = sw_pos;
      }

      /* Newline if this isn't the first line */
      if (*bufc != bb_p) {
	safe_crlf(buff, bufc);
      }

      /* Left border text */
      safe_str(l_fill, buff, bufc);

      /* Left space padding if needed */
      if (just == JUST_RIGHT) {
	nleft = width - el_pos + sl_pos;
	print_padding(nleft, max, ' ');
      } else if (just == JUST_CENTER) {
	lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);
	print_padding(lead_chrs, max, ' ');
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
      if (just == JUST_LEFT) {
	nleft = width - el_pos + sl_pos;
	print_padding(nleft, max, ' ');
      } else if (just == JUST_CENTER) {
	nleft = width - lead_chrs - el_pos + sl_pos;
	print_padding(nleft, max, ' ');
      }

      /* Right border text */
      safe_str(r_fill, buff, bufc);

      /* Update pointers for the next line */
      if (!*el) {
	/* ES, and nothing left to output */
	break;
      } else if (*ew == '\n' && sw == ew) {
	/* EL already handled on this line, and no new word yet */
	++ew;
	sl = el = NULL;
      } else if (sl == sw) {
	/* No new word yet */
	sl = el = NULL;
      } else {
	/* ES with more to output, EL for next line, or just a full line */
	sl = sw;
	sl_ansi_state = sw_ansi_state;
	sl_pos = sw_pos;
	el = ew;
	el_ansi_state = ew_ansi_state;
	el_pos = ew_pos;
      }
    }
}

/* ---------------------------------------------------------------------------
 * Misc functions.
 */

FUNCTION(fun_cat)
{
	int i;

	safe_str(fargs[0], buff, bufc);
	for (i = 1; i < nfargs; i++) {
		safe_chr(' ', buff, bufc);
		safe_str(fargs[i], buff, bufc);
	}
}

FUNCTION(fun_strcat)
{
	int i;
	
	safe_str(fargs[0], buff, bufc);
	for (i = 1; i < nfargs; i++) {
		safe_str(fargs[i], buff, bufc);
	}
}

FUNCTION(fun_strlen)
{
	safe_ltos(buff, bufc, strip_ansi_len(fargs[0]));
}

FUNCTION(fun_delete)
{
    char *s, *savep;
    int count, start, nchars;
    int ansi_state_l = ANST_NORMAL;
    int ansi_state_r = ANST_NORMAL;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);

    if ((nchars <= 0) || (start + nchars <= 0)) {
	safe_str(s, buff, bufc);
	return;
    }

    savep = s;
    for (count = 0; (count < start) && *s; ++count) {
	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state_l);
	}

	if (*s) {
	    ++s;
	}
    }

    safe_known_str(savep, s - savep, buff, bufc);
    ansi_state_r = ansi_state_l;

    while (*s == ESC_CHAR) {
	track_esccode(s, ansi_state_r);
    }

    for ( ; (count < start + nchars) && *s; ++count) {
	++s;

	while (*s == ESC_CHAR) {
	    track_esccode(s, ansi_state_r);
	}
    }

    if (*s) {
	safe_str(ansi_transition_esccode(ansi_state_l, ansi_state_r),
		 buff, bufc);
	safe_str(s, buff, bufc);
    } else {
	safe_str(ansi_transition_esccode(ansi_state_l, ANST_NORMAL),
		 buff, bufc);
    }
}

/* ---------------------------------------------------------------------------
 * Misc PennMUSH-derived functions.
 */

FUNCTION(fun_lit)
{
	/* Just returns the argument, literally */
	safe_str(fargs[0], buff, bufc);
}

FUNCTION(fun_art)
{
	/* checks a word and returns the appropriate article, "a" or "an" */
	char *s = fargs[0];
	char c;

	while (*s && (isspace(*s) || iscntrl(*s))) {
		if (*s == ESC_CHAR) {
			skip_esccode(s);
		} else {
			++s;
		}
	}

	c = tolower(*s);

	if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
		safe_known_str("an", 2, buff, bufc);
	} else {
		safe_chr('a', buff, bufc);
	}
}

FUNCTION(fun_alphamax)
{
	char *amax;
	int i = 1;

	if (!fargs[0]) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	} else
		amax = fargs[0];

	while ((i < nfargs) && fargs[i]) {
		amax = (strcmp(amax, fargs[i]) > 0) ? amax : fargs[i];
		i++;
	}

	safe_str(amax, buff, bufc);
}

FUNCTION(fun_alphamin)
{
	char *amin;
	int i = 1;

	if (!fargs[0]) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	} else
		amin = fargs[0];

	while ((i < nfargs) && fargs[i]) {
		amin = (strcmp(amin, fargs[i]) < 0) ? amin : fargs[i];
		i++;
	}

	safe_str(amin, buff, bufc);
}

FUNCTION(fun_valid)
{
/* Checks to see if a given <something> is valid as a parameter of a
 * given type (such as an object name).
 */

	if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1]) {
		safe_chr('0', buff, bufc);
	} else if (!strcasecmp(fargs[0], "name")) {
		safe_bool(buff, bufc, ok_name(fargs[1]));
	} else if (!strcasecmp(fargs[0], "attrname")) {
		safe_bool(buff, bufc, ok_attr_name(fargs[1]));
	} else if (!strcasecmp(fargs[0], "playername")) {
		safe_bool(buff, bufc, (ok_player_name(fargs[1]) &&
				       badname_check(fargs[1])));
	} else {
		safe_nothing(buff, bufc);
	}
}

FUNCTION(fun_beep)
{
	safe_chr(BEEP_CHAR, buff, bufc);
}
