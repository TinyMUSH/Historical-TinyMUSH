/*
 * stringutil.c -- string utilities 
 */
/*
 * $Id$ 
 */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "alloc.h"
#include "ansi.h"

#ifdef __linux__
char *___strtok;

#endif

/* Convert raw character sequences into MUSH substitutions (type = 1)
 * or strips them (type = 0). */

char *translate_string(str, type)
const char *str;
int type;
{
	char old[LBUF_SIZE];
	static char new[LBUF_SIZE];
	char *j, *c, *bp, *p;
	int i;

	bp = new;
	StringCopy(old, str);
		
	for (j = old; *j != '\0'; j++) {
	    switch (*j) {
		case ESC_CHAR:
		    c = (char *) index(j, ANSI_END);
		    if (c) {
			if (!type) {
			    j = c;
			    break;
			}
			*c = '\0';
			j += 2;
			
                       /* j points to the beginning of the string.
			* c points to the end of the string. Between
			* them is a set of numbers separated by
			* semicolons.
			*/

			do {
			    p = (char *) index(j, ';');
			    if (p)
				*p++ = '\0';
			    i = atoi(j);
			    switch (i) {
				case I_ANSI_NORMAL:
					safe_str("%xn", new, &bp);
					break;
				case I_ANSI_HILITE:
					safe_str("%xh", new, &bp);
					break;
				case I_ANSI_BLINK:
					safe_str("%xf", new, &bp);
					break;
				case I_ANSI_INVERSE:
					safe_str("%xi", new, &bp);
					break;
				case I_ANSI_UNDER:
					safe_str("%xu", new, &bp);
					break;
				case I_ANSI_BLACK:
					safe_str("%xx", new, &bp);
					break;
				case I_ANSI_RED:
					safe_str("%xr", new, &bp);
					break;
				case I_ANSI_GREEN:
					safe_str("%xg", new, &bp);
					break;
				case I_ANSI_YELLOW:
					safe_str("%xy", new, &bp);
					break;
				case I_ANSI_BLUE:
					safe_str("%xb", new, &bp);
					break;
				case I_ANSI_MAGENTA:
					safe_str("%xm", new, &bp);
					break;
				case I_ANSI_CYAN:
					safe_str("%xc", new, &bp);
					break;
				case I_ANSI_WHITE:
					safe_str("%xw", new, &bp);
					break;
				case I_ANSI_BBLACK:
					safe_str("%xX", new, &bp);
					break;
				case I_ANSI_BRED:
					safe_str("%xR", new, &bp);
					break;
				case I_ANSI_BGREEN:
					safe_str("%xG", new, &bp);
					break;
				case I_ANSI_BYELLOW:
					safe_str("%xY", new, &bp);
					break;
				case I_ANSI_BBLUE:
					safe_str("%xB", new, &bp);
					break;
				case I_ANSI_BMAGENTA:
					safe_str("%xM", new, &bp);
					break;
				case I_ANSI_BCYAN:
					safe_str("%xC", new, &bp);
					break;
				case I_ANSI_BWHITE:
					safe_str("%xW", new, &bp);
					break;
			    }
			    j = p;
			} while (p && *p);
			j = c;
		    } else {
			safe_chr(*j, new, &bp);
		    }
		    break;
		case ' ':
		    if ((*(j+1) == ' ') && type)
			safe_str("%b", new, &bp);
		    else 
			safe_chr(' ', new, &bp);
		    break;
		case '\\':
		    if (type)
			safe_str("%\\", new, &bp);
		    else
			safe_chr('\\', new, &bp);
		    break;
		case '%':
		    if (type)
			safe_str("%%", new, &bp);
		    else
			safe_chr('%', new, &bp);
		    break;
		case '[':
		    if (type)
			safe_str("%[", new, &bp);
		    else
			safe_chr('[', new, &bp);
		    break;
		case ']':
		    if (type)
			safe_str("%]", new, &bp);
		    else
			safe_chr(']', new, &bp);
		    break;
		case '{':
		    if (type)
			safe_str("%{", new, &bp);
		    else
			safe_chr('{', new, &bp);
		    break;
		case '}':
		    if (type)
			safe_str("%}", new, &bp);
		    else
			safe_chr('}', new, &bp);
		    break;
		case '(':
		    if (type)
			safe_str("%(", new, &bp);
		    else
			safe_chr('(', new, &bp);
		    break;
		case ')':
		    if (type)
			safe_str("%)", new, &bp);
		    else
			safe_chr(')', new, &bp);
		    break;
		case '\r':
		    break;
		case '\n':
		    if (type)
			safe_str("%r", new, &bp);
		    else
			safe_chr(' ', new, &bp);
		    break;
		default:
		    safe_chr(*j, new, &bp);
		}
	}
	*bp = '\0';
	return new;
}

/*
 * capitalizes an entire string
 */

char *upcasestr(s)
char *s;
{
	char *p;

	for (p = s; p && *p; p++)
		*p = ToUpper(*p);
	return s;
}

/*
 * returns a pointer to the next character in s matching c, or a pointer to
 * the \0 at the end of s.  Yes, this is a lot like index, but not exactly.
 */
char *seek_char(s, c)
const char *s;
char c;
{
	char *cp;

	cp = (char *)s;
	while (cp && *cp && (*cp != c))
		cp++;
	return (cp);
}

/*
 * ---------------------------------------------------------------------------
 * * munge_space: Compress multiple spaces to one space, also remove leading and
 * * trailing spaces.
 */

char *munge_space(string)
char *string;
{
	char *buffer, *p, *q;

	buffer = alloc_lbuf("munge_space");
	p = string;
	q = buffer;
	while (p && *p && isspace(*p))
		p++;		/*
				 * remove inital spaces 
				 */
	while (p && *p) {
		while (*p && !isspace(*p))
			*q++ = *p++;
		while (*p && isspace(*++p)) ;
		if (*p)
			*q++ = ' ';
	}
	*q = '\0';		/*
				 * remove terminal spaces and terminate
				 * string 
				 */
	return (buffer);
}

/*
 * ---------------------------------------------------------------------------
 * * trim_spaces: Remove leading and trailing spaces.
 */

char *trim_spaces(string)
char *string;
{
	char *buffer, *p, *q;

	buffer = alloc_lbuf("trim_spaces");
	p = string;
	q = buffer;
	while (p && *p && isspace(*p))	/* remove inital spaces */
		p++;
	while (p && *p) {
		while (*p && !isspace(*p))	/* copy nonspace chars */
			*q++ = *p++;
		while (*p && isspace(*p))	/* compress spaces */
			p++;
		if (*p)
			*q++ = ' ';	/* leave one space */ 
	}
	*q = '\0';		/* terminate string */
	return (buffer);
}

/*
 * ---------------------------------------------------------------------------
 * grabto: Return portion of a string up to the indicated character.  Also
 * returns a modified pointer to the string ready for another call.
 */

char *grabto(str, targ)
char **str, targ;
{
	char *savec, *cp;

	if (!str || !*str || !**str)
		return NULL;

	savec = cp = *str;
	while (*cp && *cp != targ)
		cp++;
	if (*cp)
		*cp++ = '\0';
	*str = cp;
	return savec;
}

int string_compare(s1, s2)
const char *s1, *s2;
{
#ifndef STANDALONE
	if (!mudconf.space_compress) {
		while (*s1 && *s2 && ToLower(*s1) == ToLower(*s2))
			s1++, s2++;

		return (ToLower(*s1) - ToLower(*s2));
	} else {
#endif
		while (isspace(*s1))
			s1++;
		while (isspace(*s2))
			s2++;
		while (*s1 && *s2 && ((ToLower(*s1) == ToLower(*s2)) ||
				      (isspace(*s1) && isspace(*s2)))) {
			if (isspace(*s1) && isspace(*s2)) {	/* skip all 
								 * other spaces 
								 */
				while (isspace(*s1))
					s1++;
				while (isspace(*s2))
					s2++;
			} else {
				s1++;
				s2++;
			}
		}
		if ((*s1) && (*s2))
			return (1);
		if (isspace(*s1)) {
			while (isspace(*s1))
				s1++;
			return (*s1);
		}
		if (isspace(*s2)) {
			while (isspace(*s2))
				s2++;
			return (*s2);
		}
		if ((*s1) || (*s2))
			return (1);
		return (0);
#ifndef STANDALONE
	}
#endif
}

int string_prefix(string, prefix)
const char *string, *prefix;
{
	int count = 0;

	while (*string && *prefix && ToLower(*string) == ToLower(*prefix))
		string++, prefix++, count++;
	if (*prefix == '\0')	/* Matched all of prefix */
		return (count);
	else
		return (0);
}

/*
 * accepts only nonempty matches starting at the beginning of a word 
 */

const char *string_match(src, sub)
const char *src, *sub;
{
	if ((*sub != '\0') && (src)) { 
		while (*src) {
			if (string_prefix(src, sub))
				return src;
			/* else scan to beginning of next word */
			while (*src && isalnum(*src))
				src++;
			while (*src && !isalnum(*src))
				src++;
		}
	}
	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * replace_string: Returns an lbuf containing string STRING with all occurances
 * of OLD replaced by NEW. OLD and NEW may be different lengths.
 * (mitch 1 feb 91)
 * replace_string_ansi: Like replace_string, but sensitive about ANSI codes.
 */

char *replace_string(old, new, string)
const char *old, *new, *string;
{
	char *result, *r, *s;
	int olen;

	if (string == NULL)
		return NULL;
	s = (char *)string;
	olen = strlen(old);
	r = result = alloc_lbuf("replace_string");
	while (*s) {

		/* Copy up to the next occurrence of the first char of OLD */

		while (*s && *s != *old) {
			safe_chr(*s, result, &r);
			s++;
		}

		/*
		 * If we are really at an OLD, append NEW to the result and
		 * bump the input string past the occurrence of
		 * OLD. Otherwise, copy the char and try again. 
		 */

		if (*s) {
			if (!strncmp(old, s, olen)) {
				safe_str((char *)new, result, &r);
				s += olen;
			} else {
				safe_chr(*s, result, &r);
				s++;
			}
		}
	}
	*r = '\0';
	return result;
}

char *replace_string_ansi(old, new, string)
    const char *old, *new, *string;
{
    char *result, *r, *s, *t, *savep;
    int count, olen, have_normal, new_ansi;
    char oldbuf[LBUF_SIZE], newbuf[LBUF_SIZE];

    if (!string)
	return NULL;

    s = (char *) string;
    r = result = alloc_lbuf("replace_string_ansi");

    olen = strlen(old);

    /* Scan the contents of the string. Figure out whether we have any
     * embedded ANSI codes.
     */
    new_ansi = (((char *) index(new, ESC_CHAR)) == NULL) ? 0 : 1;

    have_normal = 1;
    while (*s) {

	/* Copy up to the next occurrence of the first char of OLD. */

	while (*s && (*s != *old)) {
	    if (*s == ESC_CHAR) {
		/* Start of an ANSI code. Skip to the end. */
		savep = s;
		while (*s && (*s != ANSI_END)) {
		    safe_chr(*s, result, &r);
		    s++;
		}
		if (*s) {
		    safe_chr(*s, result, &r);
		    s++;
		}
		if (!strncmp(savep, ANSI_NORMAL, 4))
		    have_normal = 1;
		else
		    have_normal = 0;
	    } else {
		safe_chr(*s, result, &r);
		s++;
	    }
	}

	/* If we are really at an OLD, append NEW to the result and
	 * bump the input string past the occurrence of OLD. Otherwise,
	 * copy the char and try again.
	 */

	if (*s) {
	    if (!strncmp(old, s, olen)) {

		/* If the string contains no ANSI characters, we can
		 * just copy it. Otherwise we need to scan through it.
		 */

		if (!new_ansi) {
		    safe_str((char *) new, result, &r);
		} else {
		    t = (char *) new;
		    while (*t) {
			if (*t == ESC_CHAR) {
			    /* Start of an ANSI code. Skip to the end. */
			    savep = t;
			    while (*t && (*t != ANSI_END)) {
				safe_chr(*t, result, &r);
				t++;
			    }
			    if (*t) {
				safe_chr(*t, result, &r);
				t++;
			    }
			    if (!strncmp(savep, ANSI_NORMAL, 4))
				have_normal = 1;
			    else
				have_normal = 0;
			} else {
			    safe_chr(*t, result, &r);
			    t++;
			}
		    }
		}
		s += olen;
	    } else {
		/* We have to handle the case where the first character
		 * in OLD is the ANSI escape character. In that case
		 * we move over and copy the entire ANSI code. Otherwise
		 * we just copy the character.
		 */
		if (*old == ESC_CHAR) {
		    savep = s;
		    while (*s && (*s != ANSI_END)) {
			safe_chr(*s, result, &r);
			s++;
		    }
		    if (*s) {
			safe_chr(*s, result, &r);
			s++;
		    }
		    if (!strncmp(savep, ANSI_NORMAL, 4))
			have_normal = 1;
		    else
			have_normal = 0;
		} else {
		    safe_chr(*s, result, &r);
		    s++;
		}
	    }
	}
    }

    if (!have_normal)
	safe_str(ANSI_NORMAL, result, &r);

    *r = '\0';
    return result;
}

int minmatch(str, target, min)
char *str, *target;
int min;
{
	while (*str && *target && (ToLower(*str) == ToLower(*target))) {
		str++;
		target++;
		min--;
	}
	if (*str)
		return 0;
	if (!*target)
		return 1;
	return ((min <= 0) ? 1 : 0);
}

INLINE char *strsave(s)
const char *s;
{
	char *p;
	p = (char *)XMALLOC(sizeof(char) * (strlen(s) + 1), "strsave");

	if (p)
		strcpy(p, s);
	return p;
}

/* ---------------------------------------------------------------------------
 * safe_copy_str, safe_copy_long_str, safe_chr_real_fn - Copy buffers, 
 * watching for overflows.
 */

int 
safe_copy_str(src, buff, bufp, max)
    char *src, *buff, **bufp;
    int max;
{
    char *tp, *maxtp, *longtp;
    int n, len;

    tp = *bufp;
    if (src == NULL) {
	*tp = '\0';
	return 0;
    }
    maxtp = buff + max;
    longtp = tp + 7;
    maxtp = (maxtp < longtp) ? maxtp : longtp;

    while (*src && (tp < maxtp))
	*tp++ = *src++;

    if (*src == '\0') {
	*bufp = tp;
	if ((tp - buff) < max)
	    *tp = '\0';
	else
	    buff[max] = '\0';
	return 0;
    }

    len = strlen(src);
    n = max - (tp - buff);
    if (n <= 0) {
	*tp = '\0';
	return (len);
    }
    n = ((len < n) ? len : n);
    bcopy(src, tp, n);
    tp += n;
    *tp = '\0';
    *bufp = tp;

    return (len - n);
}

int 
safe_copy_long_str(src, buff, bufp, max)
    char *src, *buff, **bufp;
    int max;
{
    int len, n;
    char *tp;

    tp = *bufp;
    if (src == NULL) {
	*tp = '\0';
	return 0;
    }

    len = strlen(src);
    n = max - (tp - buff);
    if (n < 0)
	 n = 0;

    strncpy (tp, src, n);
    buff[max] = '\0';

    if (len <= n) {
	*bufp = tp + len;
	return (0);
    } else {
	*bufp = tp + n;
	return (len-n);
    }
}

INLINE int 
safe_chr_real_fn(src, buff, bufp, max)
    char src, *buff, **bufp;
    int max;
{
    char *tp;
    int retval = 0;

    tp = *bufp;
    if ((tp - buff) < max) {
	*tp++ = src;
	*bufp = tp;
	*tp = '\0';
    } else {
	buff[max] = '\0';
	retval = 1;
    }

    return retval;
}

int matches_exit_from_list(str, pattern)
char *str, *pattern;
{
	char *s;

	while (*pattern) {
		for (s = str;	/* check out this one */
		     (*s && (ToLower(*s) == ToLower(*pattern)) &&
		      *pattern && (*pattern != EXIT_DELIMITER));
		     s++, pattern++) ;

		/* Did we match it all? */

		if (*s == '\0') {

			/* Make sure nothing afterwards */

			while (*pattern && isspace(*pattern))
				pattern++;

			/* Did we get it? */

			if (!*pattern || (*pattern == EXIT_DELIMITER))
				return 1;
		}
		/* We didn't get it, find next string to test */

		while (*pattern && *pattern++ != EXIT_DELIMITER) ;
		while (isspace(*pattern))
			pattern++;
	}
	return 0;
}

int ltos(s, num)
char *s;
long num;
{
	/* Mark Vasoll's long int to string converter. */
	char buf[20], *p;
	long anum;

	p = buf;

	/* absolute value */
	anum = (num < 0) ? -num : num;

	/* build up the digits backwards by successive division */
	while (anum > 9) {
		*p++ = '0' + (anum % 10);
		anum /= 10;
	}

	/* put in the sign if needed */
	if (num < 0)
		*s++ = '-';

	/* put in the last digit, this makes very fast single digits numbers */
	*s++ = '0' + anum;

	/* reverse the rest of the digits (if any) into the provided buf */
	while (p-- > buf)
		*s++ = *p;

	/* terminate the resulting string */
	*s = '\0';
	return 0;
}

int safe_ltos(s, bufc, num)
char *s;
char **bufc;
long num;
{
	/* Mark Vasoll's long int to string converter. */
	char buf[20], *p;
	long anum;

	p = buf;

	/* absolute value */
	anum = (num < 0) ? -num : num;

	/* build up the digits backwards by successive division */
	while (anum > 9) {
		*p++ = '0' + (anum % 10);
		anum /= 10;
	}

	/* put in the sign if needed */
	if (num < 0)
		*(*bufc)++ = '-';

	/* put in the last digit, this makes very fast single digits numbers */
	*(*bufc)++ = '0' + anum;

	/* reverse the rest of the digits (if any) into the provided buf */
	while ((p-- > buf) && ((*bufc - s) < LBUF_SIZE))
		*(*bufc)++ = *p;

	/* terminate the resulting string */
	**bufc = '\0';
	return 0;
}
