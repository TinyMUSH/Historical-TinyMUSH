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
	EXTFUNCS *xperms;	/* Extended access to function */
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
    char str[MAX_DELIM_LEN];
} Delim;

typedef struct var_entry VARENT;
struct var_entry {
    char *text;			/* variable text */
};

typedef struct component_def COMPONENT;
struct component_def {
    int (*typer_func)();	/* type-checking handler */
    char *def_val;		/* default value */
};

typedef struct structure_def STRUCTDEF;
struct structure_def {
    char *s_name;		/* name of the structure */
    char **c_names;		/* array of component names */
    COMPONENT **c_array;	/* array of pointers to components */
    int c_count;		/* number of components */
    char delim;			/* output delimiter when unloading */
    int need_typecheck;		/* any components without types of any? */
    int n_instances;		/* number of instances out there */
    char *names_base;		/* pointer for later freeing */
    char *defs_base;		/* pointer for later freeing */
};

typedef struct instance_def INSTANCE;
struct instance_def {
    STRUCTDEF *datatype;	/* pointer to structure data type def */
};

typedef struct data_def STRUCTDATA;
struct data_def {
    char *text;
};

typedef struct object_stack STACK;
struct object_stack {
	char *data;
	STACK *next;
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

extern Delim SPACE_DELIM;

extern char *FDECL(trim_space_sep, (char *, Delim, int));
extern char *FDECL(next_token, (char *, Delim, int));
extern char *FDECL(split_token, (char **, Delim, int));
extern int FDECL(countwords, (char *, Delim, int));
extern int FDECL(list2arr, (char **, int, char *, Delim, int));
extern void FDECL(arr2list, (char **, int, char *, char **, Delim, int));
extern int FDECL(list2ansi, (char **, char *, int, char *, Delim));
extern double NDECL(makerandom);
extern int FDECL(fn_range_check, (const char *, int, int, int, char *, char **));
extern int FDECL(delim_check, (char **, int, int, Delim *, char *, char **,
			       dbref, dbref, dbref, char **, int, int));
extern INLINE void FDECL(do_reverse, (char *, char *));

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
 */

/* Separator checking "helper" macros.
 *   VaChk_Sep(sep_ptr, sep_len, arg_n, flags): Use arg_n as separator.
 *   VaChk_InSep(arg_number, flags): Use arg_number as input sep.
 *   VaChk_DefaultOut(arg_number): If nfargs less than arg_number,
 *     use the input separator. DO NOT PUT A SEMI-COLON AFTER THIS MACRO.
 *   VaChk_OutSep(arg_number, flags): Use arg_number as output sep.
 */

#define VaChk_Sep(xsep, xlen, xargnum, xflags) \
if (!(xlen = delim_check(fargs, nfargs, xargnum, xsep, buff, bufc, \
    player, caller, cause, cargs, ncargs, xflags))) \
    return;

#define VaChk_InSep(xargnum, xflags) \
VaChk_Sep(&isep, isep_len, xargnum, (xflags)|DELIM_STRING)

#define VaChk_DefaultOut(xargnum) \
if (nfargs < xargnum) { \
    if (isep_len == 1) { \
        osep.c = isep.c; \
        osep_len = 1; \
    } else { \
        strcpy(osep.str, isep.str); \
        osep_len = isep_len; \
    } \
} else

#define VaChk_OutSep(xargnum, xflags) \
VaChk_Sep(&osep, osep_len, xargnum, \
          (xflags)|DELIM_NULL|DELIM_CRLF|DELIM_STRING)

/*
 * VaChk_Range("FUNCTION", min_args, max_args): Functions which take
 *   between min_args and max_args. Don't check for delimiters.
 *
 * VaChk_Only_InPure("FUNCTION", max_args): Functions which take max_args - 1
 *   args or, with a delimiter, max_args args. No special stuff.
 * 
 * VaChk_Only_In("FUNCTION", max_args): Functions which take max_args - 1 args
 *   or, with a delimiter, max_args args.
 *
 * VaChk_Only_Out("FUNCTION", max_args): Functions which take max_args - 1 args
 *   or, with a delimiter, max_args args. The one delimiter is an output delim.
 *
 * VaChk_InPure("FUNCTION", max_args): Functions which take max_args - 1
 *   args or, with a delimiter, max_args args. No special stuff.
 *
 * VaChk_In("FUNCTION", min_args, max_args): Functions which take
 *   between min_args and max_args, with max_args as a delimiter.
 *
 * VaChk_Out("FUNCTION", min_args, max_args): Functions which take
 *   between min_args and max_args, with max_args as an output delimiter.
 *
 * VaChk_Only_In_Out("FUNCTION", max_args): Functions which take at least
 *   max_args - 2, with max_args - 1 as an input delimiter, and max_args as
 *   an output delimiter.
 *
 * VaChk_In_Out("FUNCTION", min_args, max_args): Functions which take at
 *   least min_args, with max_args - 1 as an input delimiter, and max_args
 *   as an output delimiter.
 *
 * VaChk_InEval_OutEval("FUNCTION", min_args, max_args): Functions which
 *   take at least min_args, with max_args - 1 as an input delimiter that
 *   must be evaluated, and max_args as an output delimiter which must
 *   be evaluated.
 */

#define VaChk_Range(xname,xminargs,xnargs) \
  if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc)) \
      return;

#define VaChk_Only_InPure(xname,xnargs) \
  VaChk_Range(xname, xnargs-1, xnargs) \
  if (!delim_check(fargs, nfargs, xnargs, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return;

#define VaChk_Only_In(xname,xnargs) \
  VaChk_Range(xname, xnargs-1, xnargs) \
  VaChk_InSep(xnargs, 0)

#define VaChk_Only_Out(xname,xnargs) \
  VaChk_Range(xname, xnargs-1, xnargs); \
  VaChk_OutSep(xnargs, 0)

#define VaChk_InPure(xname,xminargs,xnargs) \
  VaChk_Range(xname, xminargs, xnargs) \
  if (!delim_check(fargs, nfargs, xnargs, &isep, buff, bufc, \
    player, caller, cause, cargs, ncargs, 0)) \
    return;

#define VaChk_In(xname,xminargs,xnargs) \
  VaChk_Range(xname, xminargs, xnargs) \
  VaChk_InSep(xnargs, 0)

#define VaChk_Out(xname,xminargs,xnargs) \
  VaChk_Range(xname, xminargs, xnargs) \
  VaChk_OutSep(xnargs, 0)

#define VaChk_Only_In_Out(xname,xnargs) \
  VaChk_Range(xname, xnargs-2, xnargs) \
  VaChk_InSep(xnargs-1, 0) \
  VaChk_DefaultOut(xnargs) \
    VaChk_OutSep(xnargs, 0)

#define VaChk_In_Out(xname,xminargs,xnargs) \
  VaChk_Range(xname, xminargs, xnargs) \
  VaChk_InSep(xnargs-1, 0) \
  VaChk_DefaultOut(xnargs) \
    VaChk_OutSep(xnargs, 0)

#define VaChk_InEval_OutEval(xname, xminargs, xnargs) \
  VaChk_Range(xname, xminargs, xnargs) \
  VaChk_InSep(xnargs-1, DELIM_EVAL) \
  VaChk_OutSep(xnargs, DELIM_EVAL)

/* ---------------------------------------------------------------------------
 * Miscellaneous macros.
 */

/* Check access to built-in function. */

#define Check_Func_Access(p,f) \
(check_access(p,(f)->perms) && \
 (!((f)->xperms) || check_mod_access(p,(f)->xperms)))

/* Trim spaces. */

#define Eat_Spaces(x)	trim_space_sep((x), SPACE_DELIM, 1)

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

#endif /* __FUNCTIONS_H */
