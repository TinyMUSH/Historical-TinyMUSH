/* stringutil.c - string utilities */
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

/* ---------------------------------------------------------------------------
 * ANSI character-to-number translation table.
 */

char *ansi_nchartab[256] =
{
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,               0,               N_ANSI_BBLUE,    N_ANSI_BCYAN,
    0,               0,               0,               N_ANSI_BGREEN,
    0,               0,               0,               0,
    0,               N_ANSI_BMAGENTA, 0,               0,
    0,               0,               N_ANSI_BRED,     0,
    0,               0,               0,               N_ANSI_BWHITE,
    N_ANSI_BBLACK,   N_ANSI_BYELLOW,  0,               0,
    0,               0,               0,               0,
    0,               0,               N_ANSI_BLUE,     N_ANSI_CYAN,
    0,               0,               N_ANSI_BLINK,    N_ANSI_GREEN,
    N_ANSI_HILITE,   N_ANSI_INVERSE,  0,               0,
    0,               N_ANSI_MAGENTA,  N_ANSI_NORMAL,   0,
    0,               0,               N_ANSI_RED,      0,
    0,               N_ANSI_UNDER,    0,               N_ANSI_WHITE,
    N_ANSI_BLACK,    N_ANSI_YELLOW,   0,               0,
    0,               0,               0,               0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};

/* ---------------------------------------------------------------------------
 * ANSI number-to-character translation table.
 */

char ansi_lettab[I_ANSI_NUM] =
{
	'\0',	'h',	'\0',	'\0',	'u',	'f',	'\0',	'i',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'x',	'r',
	'g',	'y',	'b',	'm',	'c',	'w',	'\0',	'\0',
	'X',	'R',	'G',	'Y',	'B',	'M',	'C',	'W'
};

/* ---------------------------------------------------------------------------
 * ANSI packed state definitions -- number-to-bitmask translation table.
 */

int ansi_mask_bits[I_ANSI_LIM] = {
	0xfff, 0x100, 0x100, 0,     0x200, 0x400, 0,     0x800, 0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0x100, 0x100, 0,     0x200, 0x400, 0,     0x800, 0,     0,
	0x00f, 0x00f, 0x00f, 0x00f, 0x00f, 0x00f, 0x00f, 0x00f, 0,     0,
	0x0f0, 0x0f0, 0x0f0, 0x0f0, 0x0f0, 0x0f0, 0x0f0, 0x0f0, 0,     0
};

/* ---------------------------------------------------------------------------
 * ANSI packed state definitions -- number-to-bitvalue translation table.
 */

int ansi_bits[I_ANSI_LIM] = {
	0x099, 0x100, 0x000, 0,     0x200, 0x400, 0,     0x800, 0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0x000, 0x000, 0,     0x000, 0x000, 0,     0x000, 0,     0,
	0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0,     0,
	0x000, 0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070, 0,     0
};

/* ---------------------------------------------------------------------------
 * strip_ansi -- return a new string with escape codes removed
 */

char *strip_ansi(s)
const char *s;
{
	static char buf[LBUF_SIZE];
	char *p = buf;

	if (s) {
	  while (*s == ESC_CHAR) {
	    skip_esccode(s);
	  }

	  while (*s) {
	    *p++ = *s++;
	    while (*s == ESC_CHAR) {
	      skip_esccode(s);
	    }
	  }
	}

	*p = '\0';
	return buf;
}

/* ---------------------------------------------------------------------------
 * strip_ansi_len -- count non-escape-code characters
 */

int strip_ansi_len(s)
const char *s;
{
	int n = 0;

	if (s) {
		while (*s == ESC_CHAR) {
			skip_esccode(s);
		}

		while (*s) {
			++s, ++n;
			while (*s == ESC_CHAR) {
				skip_esccode(s);
			}
		}
	}

	return n;
}

char *normal_to_white(raw)
const char *raw;
{
	static char buf[LBUF_SIZE];
	char *p = (char *)raw;
	char *q = buf;


	while (p && *p) {
		if (*p == ESC_CHAR) {
			/* Start of ANSI code. */
			*q++ = *p++;	/* ESC CHAR */
			*q++ = *p++;	/* [ character. */
			if (*p == '0') {	/* ANSI_NORMAL */
				safe_known_str("0m", 2, buf, &q);
				safe_chr(ESC_CHAR, buf, &q);
				safe_known_str("[37m", 4, buf, &q);
				p += 2;
			}
		} else
			*q++ = *p++;
	}
	*q = '\0';
	return buf;
}

char *ansi_transition_esccode(ansi_before, ansi_after)
int ansi_before, ansi_after;
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	static char buffer[64];

	buffer[0] = ESC_CHAR;
	buffer[1] = '[';
	p = buffer + 2;

	/* If they turn off any highlight bits, or they change from some color
	 * to default color, we need to use ansi normal first.
	 */

	ansi_bits_set = (~ansi_before) & ansi_after;
	ansi_bits_clr = ansi_before & (~ansi_after);
	if ((ansi_bits_clr & 0xf00) || (ansi_bits_set & 0x088)) {
		strcpy(p, "0;"); p += 2;
		ansi_bits_set = (~ansi_bits[0]) & ansi_after;
		ansi_bits_clr = ansi_bits[0] & (~ansi_after);
	}

	/* Next reproduce the highlight state */

	if (ansi_bits_set & 0x100) {
		strcpy(p, "1;"); p += 2;
	}
	if (ansi_bits_set & 0x200) {
		strcpy(p, "4;"); p += 2;
	}
	if (ansi_bits_set & 0x400) {
		strcpy(p, "5;"); p += 2;
	}
	if (ansi_bits_set & 0x800) {
		strcpy(p, "7;"); p += 2;
	}

	/* Foreground color */
	if ((ansi_bits_set | ansi_bits_clr) & 0x00f) {
		strcpy(p, "30;"); p += 3;
		p[-2] |= (ansi_after & 0x00f);
	}

	/* Background color */
	if ((ansi_bits_set | ansi_bits_clr) & 0x0f0) {
		strcpy(p, "40;"); p += 3;
		p[-2] |= ((ansi_after & 0x0f0) >> 4);
	}

	/* Terminate */
	if (p > buffer + 2) {
		p[-1] = 'm';
		/* Buffer is already null-terminated by strcpy */
	} else {
		buffer[0] = '\0';
	}
	return buffer;
}

char *ansi_transition_mushcode(ansi_before, ansi_after)
int ansi_before, ansi_after;
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	static char ansi_mushcode_fg[9] = "xrgybmcw";
	static char ansi_mushcode_bg[9] = "XRGYBMCW";
	static char buffer[64];

	p = buffer;

	/* If they turn off any highlight bits, or they change from some color
	 * to default color, we need to use ansi normal first.
	 */

	ansi_bits_set = (~ansi_before) & ansi_after;
	ansi_bits_clr = ansi_before & (~ansi_after);
	if ((ansi_bits_clr & 0xf00) || (ansi_bits_set & 0x088)) {
		strcpy(p, "%xn"); p += 3;
		ansi_bits_set = (~ansi_bits[0]) & ansi_after;
		ansi_bits_clr = ansi_bits[0] & (~ansi_after);
	}

	/* Next reproduce the highlight state */

	if (ansi_bits_set & 0x100) {
		strcpy(p, "%xh"); p += 3;
	}
	if (ansi_bits_set & 0x200) {
		strcpy(p, "%xu"); p += 3;
	}
	if (ansi_bits_set & 0x400) {
		strcpy(p, "%xf"); p += 3;
	}
	if (ansi_bits_set & 0x800) {
		strcpy(p, "%xi"); p += 3;
	}

	/* Foreground color */
	if ((ansi_bits_set | ansi_bits_clr) & 0x00f) {
		strcpy(p, "%xx"); p += 3;
		p[-1] = ansi_mushcode_fg[(ansi_after & 0x00f)];
	}

	/* Background color */
	if ((ansi_bits_set | ansi_bits_clr) & 0x0f0) {
		strcpy(p, "%xX"); p += 3;
		p[-1] = ansi_mushcode_bg[(ansi_after & 0x0f0) >> 4];
	}

	/* Terminate */
	*p = '\0';
	return buffer;
}


/* translate_string -- Convert (type = 1) raw character sequences into
 * MUSH substitutions or strip them (type = 0).
 */

char *translate_string(str, type)
char *str;
int type;
{
	static char new[LBUF_SIZE];
	char *bp, *c, *p;
	int i;

	bp = new;

	if (type) {
		int ansi_state = ANST_NORMAL;
		int ansi_state_prev = ANST_NORMAL;

		while (*str) {
			switch (*str) {
			case ESC_CHAR:
				while (*str == ESC_CHAR) {
					track_esccode(str, ansi_state);
				}
				safe_str(ansi_transition_mushcode(ansi_state_prev, ansi_state), new, &bp);
				ansi_state_prev = ansi_state;
				continue;
			case ' ':
				if (str[1] == ' ') {
					safe_known_str("%b", 2, new, &bp);
				} else {
					safe_chr(' ', new, &bp);
				}
				break;
			case '\\': case '%': case '[': case ']':
			case '{':  case '}': case '(': case ')':
				safe_chr('%', new, &bp);
				safe_chr(*str, new, &bp);
				break;
			case '\r':
				break;
			case '\n':
				safe_known_str("%r", 2, new, &bp);
				break;
			case '\t':
				safe_known_str("%t", 2, new, &bp);
				break;
			default:
				safe_chr(*str, new, &bp);
			}
			str++;
		}
	} else {
		while (*str) {
			switch (*str) {
			case ESC_CHAR:
				skip_esccode(str);
				continue;
			case '\r':
				break;
			case '\n': case '\t':
				safe_chr(' ', new, &bp);
				break;
			default:
				safe_chr(*str, new, &bp);
			}
			str++;
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
		Skip_Ansi_Code(s, result, &r);
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
			    Skip_Ansi_Code(t, result, &r);
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
		    Skip_Ansi_Code(s, result, &r);
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
    if (tp + known < maxtp)
	maxtp = tp + known;
    while (*src && (tp < maxtp))
	*(tp)++ = *src++;

    *tp = '\0';
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
	*s++ = '0' + (char)anum;

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
			*tp++ = '0' + (char)anum;

		/* reverse the rest of the digits (if any) into the
		 * provided buf
		 */
		while ((p-- > buf) && (tp < endp))
			*tp++ = *p;
	} else {
		if (num < 0)
			*tp++ = '-';
		*tp++ = '0' + (char)anum;
		while (p-- > buf)
			*tp++ = *p;
	}

	/* terminate the resulting string */
	*tp = '\0';
	*bufc = tp;
}
