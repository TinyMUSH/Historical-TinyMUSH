/* functions.h - declarations for functions & function processing */
/* $Id$ */

#include "copyright.h"

#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include <limits.h>
#include <math.h>

#include "externs.h"
#include "misc.h"
#include "attrs.h"
#include "match.h"
#include "command.h"
#include "ansi.h"

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

/* Function declarations */

extern char *FDECL(trim_space_sep, (char *, char));
extern char *FDECL(next_token, (char *, char));
extern char *FDECL(split_token, (char **, char));
extern int FDECL(countwords, (char *, char));
extern int FDECL(list2arr, (char **, int, char *, char));
extern void FDECL(arr2list, (char **, int, char *, char **, char));
extern dbref FDECL(match_thing, (dbref, char *));
extern double NDECL(makerandom);
extern int FDECL(fn_range_check, (const char *, int, int, int, char *, char **));
extern int FDECL(delim_check, (char **, int, int, char *, char *, char **, int, dbref, dbref, char **, int, int));
extern int FDECL(check_read_perms, (dbref, dbref, ATTR *, int, int, char *, char **));
extern INLINE void FDECL(do_reverse, (char *, char *));

/* Function prototype macro */

#define	FUNCTION(x)	\
	void x(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs) \
	char *buff, **bufc; \
	dbref player, cause; \
	char *fargs[], *cargs[]; \
	int nfargs, ncargs;

/* This is for functions that take an optional delimiter character.
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
 * Call xvarargs_preamble("FUNCTION", min_args, max_args) if this is varargs
 * but does not involve a delimiter.
 */

#define xvarargs_preamble(xname,xminargs,xnargs)                \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc))     \
return;

#define varargs_preamble(xname,xnargs)	                        \
if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufc))	\
return;							        \
if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufc, 0,		\
    player, cause, cargs, ncargs, 0))                              \
return;

#define mvarargs_preamble(xname,xminargs,xnargs)	        \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc))	\
return;							        \
if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufc, 0,          \
    player, cause, cargs, ncargs, 0))                              \
return;

#define evarargs_preamble(xname, xminargs, xnargs)              \
if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc))	\
return;							        \
if (!delim_check(fargs, nfargs, xnargs - 1, &sep, buff, bufc, 1,      \
    player, cause, cargs, ncargs, 0))                              \
return;							        \
if (!delim_check(fargs, nfargs, xnargs, &osep, buff, bufc, 1,         \
    player, cause, cargs, ncargs, 1))                              \
return;

#define svarargs_preamble(xname,xnargs)                         \
if (!fn_range_check(xname, nfargs, xnargs-2, xnargs, buff, bufc))	\
return;							        \
if (!delim_check(fargs, nfargs, xnargs-1, &sep, buff, bufc, 0,        \
    player, cause, cargs, ncargs, 0))                              \
return;							        \
if (nfargs < xnargs)				                \
    osep = sep;				                        \
else if (!delim_check(fargs, nfargs, xnargs, &osep, buff, bufc, 0,    \
    player, cause, cargs, ncargs, 1))                              \
return;

/* Special handling of separators. */

#define print_sep(s,b,p) \
if (s) { \
    if (s != '\r') { \
	safe_chr(s,b,p); \
    } else { \
	safe_crlf(b,p); \
    } \
}

/* Macro for finding an <attr> or <obj>/<attr>
 */

#define Parse_Uattr(p,s,t,n,a)				\
    if (parse_attrib((p), (s), &(t), &(n))) {		\
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

#endif
