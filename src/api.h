/* api.h - must be included by all dynamically loaded modules */
/* $Id */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#ifndef __API_H
#define __API_H

#include "alloc.h"      /* required by mudconf */
#include "flags.h"      /* required by mudconf */
#include "htab.h"       /* required by mudconf */
#include "mail.h"       /* required by mudconf */
#include "mudconf.h"    /* required by code */

#include "db.h"         /* required by externs */
#include "externs.h"    /* required by interface */
#include "interface.h"  /* required by code */

#include "command.h"    /* required by code */
#include "match.h"      /* required by code */
#include "attrs.h"      /* required by code */
#include "powers.h"     /* required by code */
#include "ansi.h"	/* required by code */
#include "functions.h"	/* required by code */

/* ------------------------------------------------------------------------
 * Command handler macros.
 */

#define DO_CMD_NO_ARG(name) \
	void name (player, cause, key) \
	dbref player, cause; \
	int key;

#define DO_CMD_ONE_ARG(name) \
	void name (player, cause, key, arg1) \
	dbref player, cause; \
	int key; \
	char *arg1;

#define DO_CMD_TWO_ARG(name) \
	void name (player, cause, key, arg1, arg2) \
	dbref player, cause; \
	int key; \
	char *arg1, *arg2;

#define DO_CMD_TWO_ARG_ARGV(name) \
	void name (player, cause, key, arg1, arg2_vector, vector_size) \
	dbref player, cause; \
	int key; \
	char *arg1; \
	char *arg2_vector[]; \
	int vector_size;

/* ------------------------------------------------------------------------
 * API interface functions.
 */

extern void FDECL(register_commands, (CMDENT *));
extern void FDECL(register_functions, (FUN *));
extern void FDECL(register_hashtables, (MODHASHES *, MODNHASHES *));

/* ------------------------------------------------------------------------
 * External necessities.
 */

extern CF_HDCL(cf_alias);
extern CF_HDCL(cf_bool);
extern CF_HDCL(cf_const);
extern CF_HDCL(cf_int);
extern CF_HDCL(cf_modify_bits);
extern CF_HDCL(cf_ntab_access);
extern CF_HDCL(cf_option);
extern CF_HDCL(cf_set_flags);
extern CF_HDCL(cf_string);

#endif /* __API_H */
