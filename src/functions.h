/* functions.h - declarations for functions & function processing */
/* $Id$ */

#include "copyright.h"

#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

/* ---------------------------------------------------------------------------
 * Type definitions.
 */

typedef struct fun {
	const char *name;	/* function name */
	void	(*fun)();	/* handler */
	int	nargs;		/* Number of args needed or expected */
	int	flags;		/* Function flags */
	int	perms;		/* Access to function */
} FUN;

typedef struct ufun {
	char *name;	/* function name */
	dbref	obj;		/* Object ID */
	int	atr;		/* Attribute ID */
	int	flags;		/* Function flags */
	int	perms;		/* Access to function */
	struct ufun *next;	/* Next ufun in chain */
} UFUN;

typedef union char_or_string {
    char c;
    char str[LBUF_SIZE];
} Delim;

typedef struct var_entry VARENT;
struct var_entry {
    char *text;			/* variable text */
};

#ifdef FLOATING_POINTS
#ifndef linux                   /* linux defines atof as a macro */
double atof();
#endif                          /* ! linux */
#define aton atof
typedef double NVAL;
#else
#define aton atoi
typedef int NVAL;
#endif                          /* FLOATING_POINTS */

/* ---------------------------------------------------------------------------
 * Constants.
 */

#define DELIM_EVAL	0x001	/* Must eval delimiter. */
#define DELIM_NULL	0x002	/* Null delimiter okay. */
#define DELIM_CRLF	0x004	/* '%r' delimiter okay. */
#define DELIM_STRING	0x008	/* Multi-character delimiter okay. */

/* ---------------------------------------------------------------------------
 * Function declarations.
 */

extern char *FDECL(trim_space_sep, (char *, char));
extern char *FDECL(next_token, (char *, char));
extern char *FDECL(split_token, (char **, char));
extern int FDECL(countwords, (char *, char));
extern int FDECL(list2arr, (char **, int, char *, char));
extern void FDECL(arr2list, (char **, int, char *, char **, Delim, int));
extern int FDECL(list2ansi, (char **, char *, int, char *, char));
extern double NDECL(makerandom);
extern int FDECL(fn_range_check, (const char *, int, int, int, char *, char **));
extern int FDECL(delim_check, (char **, int, int, Delim *, char *, char **,
			       dbref, dbref, dbref, char **, int, int));
extern INLINE void FDECL(do_reverse, (char *, char *));

extern char *ansi_nchartab[];	/* from fnhelper.c */

/* ---------------------------------------------------------------------------
 * Function prototype macro.
 */

#define	FUNCTION(x)	\
    void x(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs) \
	char *buff, **bufc; \
	dbref player, caller, cause; \
	char *fargs[], *cargs[]; \
	int nfargs, ncargs;

/* ---------------------------------------------------------------------------
 * Delimiter macros for functions that take an optional delimiter character.
 *
 * Call varargs_preamble("FUNCTION", max_args) for functions which
 * take either max_args - 1 args, or, with a delimiter, max_args args.
 *
 * Call mvarargs_preamble("FUNCTION", min_args, max_args) if there can
 * be more variable arguments than just the delimiter.
 *
 * Call evarargs_preamble("FUNCTION", min_args, max_args) if the delimiters
 * need to be evaluated.
 *
 * Call svarargs_preamble("FUNCTION", max_args) if the second to last and
 * last arguments are delimiters.
 *
 * Call msvarargs_preamble("FUNCTION", min_args, max_args) if there are more
 * variable arguments than just delimiters, but the second to last and last
 * arguments are delimiters.
 *
 * Call xvarargs_preamble("FUNCTION", min_args, max_args) if this is varargs
 * but does not involve a delimiter.
 */

#define xvarargs_preamble(xname,xminargs,xnargs) \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc)) \
    return;

#define varargs_preamble(xname,xnargs) \
if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufc)) \
    return; \
if (!delim_check(fargs, nfargs, xnargs, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return;

#define mvarargs_preamble(xname,xminargs,xnargs) \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc)) \
    return; \
if (!delim_check(fargs, nfargs, xnargs, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return;

#define evarargs_preamble(xname, xminargs, xnargs) \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc)) \
    return; \
if (!delim_check(fargs, nfargs, xnargs - 1, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, DELIM_EVAL)) \
    return; \
if (!(osep_len = delim_check(fargs, nfargs, xnargs, &osep, buff, bufc, \
    player, caller, cause, cargs, ncargs, DELIM_EVAL|DELIM_NULL|DELIM_CRLF))) \
    return;

#define svarargs_preamble(xname,xnargs) \
if (!fn_range_check(xname, nfargs, xnargs-2, xnargs, buff, bufc)) \
    return; \
if (!delim_check(fargs, nfargs, xnargs-1, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return; \
if (nfargs < xnargs) { \
    osep.c = isep.c; \
    osep_len = 1; \
} else if (!(osep_len = delim_check(fargs, nfargs, xnargs, &osep, \
           buff, bufc, player, caller, cause, cargs, ncargs, \
           DELIM_NULL|DELIM_CRLF))) \
    return;

#define msvarargs_preamble(xname,xminargs,xnargs) \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc)) \
    return; \
if (!delim_check(fargs, nfargs, xnargs-1, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return; \
if (nfargs < xnargs) { \
    osep.c = isep.c; \
    osep_len = 1; \
} else if (!(osep_len = delim_check(fargs, nfargs, xnargs, &osep, buff, bufc, \
    player, caller, cause, cargs, ncargs, DELIM_NULL|DELIM_CRLF))) \
    return;

/* ---------------------------------------------------------------------------
 * Miscellaneous macros.
 */

/* Special handling of separators. */

#define print_sep(s,l,b,p) \
if ((l) == 1) { \
    if ((s).c == '\r') { \
	safe_crlf((b),(p)); \
    } else if ((s).c != '\0') { \
	safe_chr((s).c,(b),(p)); \
    } \
} else { \
    safe_known_str((s).str, (l), (b), (p)); \
}

/* Macro for finding an <attr> or <obj>/<attr>
 */

#define Parse_Uattr(p,s,t,n,a)				\
    if (parse_attrib((p), (s), &(t), &(n), 0)) {	\
	if (((n) == NOTHING) || !(Good_obj(t)))		\
	    (a) = NULL;					\
	else						\
	    (a) = atr_num(n);				\
    } else {						\
        (t) = (p);					\
	(a) = atr_str(s);				\
    }

/* Macro for obtaining an attrib. */

#define Get_Uattr(p,t,a,b,o,f,l)				\
    if (!(a)) {							\
	return;							\
    }								\
    (b) = atr_pget(t, (a)->number, &(o), &(f), &(l));		\
    if (!*(b) || !(See_attr((p), (t), (a), (o), (f)))) {	\
	free_lbuf(b);						\
	return;							\
    }

/* Macro for skipping to the end of an ANSI code, if we're at the
 * beginning of one. 
 */
#define Skip_Ansi_Code(s) \
	    savep = s;				\
	    while (*s && (*s != ANSI_END)) {	\
		safe_chr(*s, buff, bufc);	\
		s++;				\
	    }					\
	    if (*s) {				\
		safe_chr(*s, buff, bufc);	\
		s++;				\
	    }					\
	    if (!strncmp(savep, ANSI_NORMAL, 4)) {	\
		have_normal = 1;		\
	    } else {				\
		have_normal = 0;		\
            }

/* Macro for writing a certain amount of padding into a buffer.
 * l is the number of characters left to write.
 * m is a throwaway integer for holding the maximum.
 * c is the separator character to use.
 */
#define print_padding(l,m,c) \
if ((l) > 0) { \
    (m) = LBUF_SIZE - 1 - (*bufc - buff); \
    (l) = ((l) > (m)) ? (m) : (l); \
    memset(*bufc, (c), (l)); \
    *bufc += (l); \
    **bufc = '\0'; \
}

/* Macro for turning an ANSI state back into ANSI codes.
 * x is a throwaway character pointer.
 * w is a pointer to the ANSI state of the previous word.
 */
#define print_ansi_state(x,w) \
safe_copy_known_str(ANSI_BEGIN, 2, buff, bufc, LBUF_SIZE - 1); \
for ((x) = (w); *(x); (x)++) { \
    if ((x) != (w)) { \
	safe_chr(';', buff, bufc); \
    } \
    safe_str(ansi_nchartab[(unsigned char) *(x)], buff, bufc); \
} \
safe_chr(ANSI_END, buff, bufc);

#endif /* __FUNCTIONS_H */
