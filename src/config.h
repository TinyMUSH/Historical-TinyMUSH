/* config.h - Configuration of compile-time options, limits, db format, etc. */
/* $Id$ */

#include "copyright.h"

#ifndef __CONFIG_H
#define __CONFIG_H

/* Do not change anything unless you are certain you know what it does.
 * General user-definable compile-time options can be found in the
 * Makefile.
 */

#define CONF_FILE "netmush.conf"	/* Default config file */
#define LOG_FILE "netmush.log"		/* Default log file */
				 
#define PLAYER_NAME_LIMIT	22	/* Max length for player names */
#define NUM_ENV_VARS		10	/* Number of env vars (%0 et al) */
#define MAX_ARG			100	/* max # args from command processor */
#define MAX_ITER_NESTING	1024	/* max # of iter levels */

#ifndef MAX_GLOBAL_REGS
#define MAX_GLOBAL_REGS		36	/* r() registers: must be 10 or 36 */ 
#endif

#define MARK_FLAG_SEP		'_'	/* sep. of dbref from marker flags */

#define HASH_FACTOR		2	/* How much hashing you want. */

#define OUTPUT_BLOCK_SIZE	16384

#define DOING_LEN		41	/* length of the DOING field in WHO */

#define PUEBLO_SUPPORT_MSG "This world is Pueblo 1.0 enhanced\r\n\r\n"

/* ---------------------------------------------------------------------------
 * Database R/W flags.
 */

#define MANDFLAGS       (V_LINK|V_PARENT|V_XFLAGS|V_ZONE|V_POWERS|V_3FLAGS|V_QUOTED|V_TQUOTAS|V_TIMESTAMPS|V_VISUALATTRS)

#define OFLAGS1		(V_GDBM|V_ATRKEY)	/* GDBM has these */

#define OFLAGS2		(V_ATRNAME|V_ATRMONEY)

#define OUTPUT_VERSION	1			/* Version 1 */
#ifdef MEMORY_BASED
#define OUTPUT_FLAGS	(MANDFLAGS)
#else
#define OUTPUT_FLAGS	(MANDFLAGS|OFLAGS1|OFLAGS2)
						/* format for dumps */
#endif /* MEMORY_BASED */

#define UNLOAD_VERSION	1			/* verison for export */
#define UNLOAD_OUTFLAGS	(MANDFLAGS)		/* format for export */

/* magic lock cookies */
#define NOT_TOKEN	'!'
#define AND_TOKEN	'&'
#define OR_TOKEN	'|'
#define LOOKUP_TOKEN	'*'
#define NUMBER_TOKEN	'#'
#define INDIR_TOKEN	'@'		/* One of these two should go. */
#define CARRY_TOKEN	'+'		/* One of these two should go. */
#define IS_TOKEN	'='
#define OWNER_TOKEN	'$'

/* matching attribute tokens */
#define AMATCH_CMD	'$'
#define AMATCH_LISTEN	'^'

/* delimiters for various things */
#define EXIT_DELIMITER	';'
#define ARG_DELIMITER	'='

/* These chars get replaced by the current item from a list in commands and
 * functions that do iterative replacement, such as @apply_marked, dolist,
 * the eval= operator for @search, and iter().
 */

#define BOUND_VAR	"##"
#define LISTPLACE_VAR	"#@"

/* This token is similar, marking the first argument in a switch. */

#define SWITCH_VAR	"#$"

/* This token is used to denote a null output delimiter. */

#define NULL_DELIM_VAR	"@@"

/* This is used to indent output from pretty-printing. */

#define INDENT_STR	"  "

/* This is used as the 'null' delimiter for structures stored via write(). */

#define GENERIC_STRUCT_DELIM		'\f'	/* form feed char */
#define GENERIC_STRUCT_STRDELIM		"\f"

/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)/mudconf.sacfactor) +mudconf.sacadjust)

/* !!! added for recycling, return value of object */
#define OBJECT_DEPOSIT(pennies) \
    (((pennies)-mudconf.sacadjust)*mudconf.sacfactor)

#define StringCopy strcpy
#define StringCopyTrunc strncpy

#define DEV_NULL "/dev/null"
#define READ read
#define WRITE write

#ifdef TEST_MALLOC
extern int malloc_count;
#define XMALLOC(x,y) (fprintf(stderr,"Malloc: %s/%d\n", (y), (x)), malloc_count++, \
                    (char *)malloc((x)))
#define XCALLOC(x,z,y) (fprintf(stderr,"Calloc: %s/%d\n", (y), (x*z)), malloc_count++, \
                    (char *)calloc((x),(z)))
#define XREALLOC(x,z,y) (fprintf(stderr,"Realloc: %s/%d\n", (y), (z)), \
                    (char *)realloc((x),(z)))
#define XSTRDUP(x,y) (fprintf(stderr,"Strdup: %s/%d\n", (y), (strlen(x))), malloc_count++, \
                    (char *)strdup((x)))
#define XSTRNDUP(x,z,y) (fprintf(stderr,"Strndup: %s/%d\n", (y), (z)),malloc_count++, \
                    (char *)strndup((x),(z)))
#define XFREE(x,y) (fprintf(stderr, "Free: %s\n", (y)), \
                    ((x) ? malloc_count--, free((void *)(x)), (x)=NULL : (x)))
#else
#define XMALLOC(x,y) (malloc(x))
#define XCALLOC(x,z,y) (calloc((x),(z)))
#define XREALLOC(x,z,y) (realloc((x),(z)))
#define XSTRDUP(x,y) (strdup(x))
#define XSTRNDUP(x,z,y) (strndup((x),(z)))
#define XFREE(x,y) (free((void *)(x)), (x) = NULL)
#endif  /* TEST_MALLOC */

#endif /* __CONFIG_H */
