/* functions.h - declarations for functions & function processing */
/* $Id$ */

#include "copyright.h"

#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

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

#define	FN_VARARGS	1	/* Function allows a variable # of args */
#define	FN_NO_EVAL	2	/* Don't evaluate args to function */
#define	FN_PRIV		4	/* Perform user-def function as holding obj */
#define FN_PRES		8	/* Preseve r-regs before user-def functions */

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
	safe_str((char *) "\r\n",b,p); \
    } \
}

extern void	NDECL(init_functab);
extern void	FDECL(list_functable, (dbref));

#endif
