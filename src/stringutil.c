/* stringutil.c - string utilities */
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

#include "ansi.h"	/* required by code */

/* Provide strtok_r (reentrant strtok) if needed */

#ifndef HAVE_STRTOK_R
char *strtok_r(s, sep, last)
char *s;
const char *sep;
char **last;
{
    if (!s)
	s = *last;
#ifdef HAVE_STRCSPN
    s += strspn(s, sep);
    *last = s + strcspn(s, sep);
#else /* HAVE_STRCSPN */
    while (strchr(sep, *s) && *s) s++;
    *last = s;
    while (!strchr(sep, **last) && **last) (*last)++;
#endif /* HAVE_STRCSPN */
    if (s == *last)
	return NULL;
    if (**last)
	*((*last)++) = '\0';
    return s;
}
#endif /* HAVE_STRTOK_R */

#define Skip_Ansi_Code(x) \
		savep = x;				\
		while (*x && (*x != ANSI_END)) {	\
		    safe_chr(*x, result, &r);		\
		    x++;				\
		}					\
		if (*x) {				\
		    safe_chr(*x, result, &r);		\
		    x++;				\
		}					\
		if (!strncmp(savep, ANSI_NORMAL, 4)) {	\
		    have_normal = 1;			\
		} else {				\
		    have_normal = 0;			\
                }


/* Convert raw character sequences into MUSH substitutions (type = 1)
 * or strips them (type = 0). */

char *ansi_numtab[I_ANSI_NUM] =
{
    "%xn", "%xh", 0,     0,     "%xu", "%xf", 0,     "%xi",
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     "%xx", "%xr",
    "%xg", "%xy", "%xb", "%xm", "%xc", "%xw", 0,     0,
    "%xX", "%xR", "%xG", "%xY", "%xB", "%xM", "%xC", "%xW"
};

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
		    c = strchr(j, ANSI_END);
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
			    p = strchr(j, ';');
			    if (p)
				*p++ = '\0';
			    i = atoi(j);
			    if (i >= 0 && i < I_ANSI_NUM && ansi_numtab[i]) {
				safe_known_str(ansi_numtab[i], 3, new, &bp);
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
		*p = toupper(*p);
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
		while (*s1 && *s2 && tolower(*s1) == tolower(*s2))
			s1++, s2++;

		return (tolower(*s1) - tolower(*s2));
	} else {
#endif
		while (isspace(*s1))
			s1++;
		while (isspace(*s2))
			s2++;
		while (*s1 && *s2 && ((tolower(*s1) == tolower(*s2)) ||
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

	while (*string && *prefix && tolower(*string) == tolower(*prefix))
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
    int olen, have_normal, new_ansi;

    if (!string)
	return NULL;

    s = (char *) string;
    r = result = alloc_lbuf("replace_string_ansi");

    olen = strlen(old);

    /* Scan the contents of the string. Figure out whether we have any
     * embedded ANSI codes.
     */
    new_ansi = strchr(new, ESC_CHAR) ? 1 : 0;

    have_normal = 1;
    while (*s) {

	/* Copy up to the next occurrence of the first char of OLD. */

	while (*s && (*s != *old)) {
	    if (*s == ESC_CHAR) {
		Skip_Ansi_Code(s);
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
			    Skip_Ansi_Code(t);
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
		    Skip_Ansi_Code(s);
		} else {
		    safe_chr(*s, result, &r);
		    s++;
		}
	    }
	}
    }

    if (!have_normal)
	safe_ansi_normal(result, &r);

    *r = '\0';
    return result;
}

int minmatch(str, target, min)
char *str, *target;
int min;
{
	while (*str && *target && (tolower(*str) == tolower(*target))) {
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

/* ---------------------------------------------------------------------------
 * safe_copy_str, safe_copy_long_str, safe_chr_real_fn - Copy buffers, 
 * watching for overflows.
 */

INLINE int 
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

    if (*src == '\0') { /* copied whole src, and tp is at most maxtp */
	*tp = '\0';
	*bufp = tp;
	return 0;
    }

    len = strlen(src);
    n = max - (tp - buff); /* tp is either maxtp or longtp */
    if (n <= 0) {
	*tp = '\0';
	return (len);
    }
    n = ((len < n) ? len : n);
    memcpy(tp, src, n);
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


INLINE void safe_copy_known_str(src, known, buff, bufp, max)
    char *src, *buff, **bufp;
    int known, max;
{
    int n;
    char *tp, *maxtp;

    tp = *bufp;

    if (!src) {
	*tp = '\0';
	return;
    }

    if (known > 7) {
	n = max - (tp - buff);
	if (n <= 0) {
	    *tp = '\0';
	    return;
	}
	n = ((known < n) ? known : n);
	memcpy(tp, src, n);
	tp += n;
	*tp = '\0';
	*bufp = tp;
	return;
    }

    maxtp = buff + max;
    while (*src && (tp < maxtp))
	*(tp)++ = *src++;

    if (!*src) {
	if (tp < maxtp)
	    *tp = '\0';
	else
	    *maxtp = '\0';
    }

    *bufp = tp;
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

/* ---------------------------------------------------------------------------
 * More utilities.
 */

int matches_exit_from_list(str, pattern)
char *str, *pattern;
{
	char *s;

	while (*pattern) {
		for (s = str;	/* check out this one */
		     (*s && (tolower(*s) == tolower(*pattern)) &&
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

INLINE void safe_ltos(s, bufc, num)
char *s;
char **bufc;
long num;
{
	/* Mark Vasoll's long int to string converter. */
	char buf[20], *p, *tp, *endp;
	long anum;

	p = buf;
	tp = *bufc;

	/* absolute value */
	anum = (num < 0) ? -num : num;

	/* build up the digits backwards by successive division */
	while (anum > 9) {
		*p++ = '0' + (anum % 10);
		anum /= 10;
	}

	if (tp > s + LBUF_SIZE - 21) {
		endp = s + LBUF_SIZE - 1;
		/* put in the sign if needed */
		if (num < 0 && (tp < endp))
			*tp++ = '-';

		/* put in the last digit, this makes very fast single 
		 * digits numbers
		 */
		if (tp < endp)
			*tp++ = '0' + anum;

		/* reverse the rest of the digits (if any) into the
		 * provided buf
		 */
		while ((p-- > buf) && (tp < endp))
			*tp++ = *p;
	} else {
		if (num < 0)
			*tp++ = '-';
		*tp++ = '0' + anum;
		while (p-- > buf)
			*tp++ = *p;
	}

	/* terminate the resulting string */
	*tp = '\0';
	*bufc = tp;
}
