/* funceval.c - More function handlers */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <limits.h>
#include <math.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "flags.h"
#include "powers.h"
#include "attrs.h"
#include "externs.h"
#include "match.h"
#include "command.h"
#include "functions.h"
#include "misc.h"
#include "alloc.h"
#include "ansi.h"
#include "db_sql.h"

extern NAMETAB indiv_attraccess_nametab[];
extern char *FDECL(trim_space_sep, (char *, char));
extern char *FDECL(next_token, (char *, char));
extern char *FDECL(split_token, (char **, char));
extern dbref FDECL(match_thing, (dbref, char *));
extern int FDECL(countwords, (char *, char));
extern int FDECL(check_read_perms, (dbref, dbref, ATTR *, int, int, char *, char **));
extern void FDECL(arr2list, (char **, int, char *, char **, char));
extern int FDECL(list2arr, (char **, int, char *, char));
extern void FDECL(make_portlist, (dbref, dbref, char *, char **));
extern INLINE char *FDECL(get_mail_message, (int));
extern void FDECL(count_mail, (dbref, int, int *, int *, int *));
extern double NDECL(makerandom);
extern int FDECL(fn_range_check, (const char *, int, int, int, char *, char **));
extern int FDECL(delim_check, (char **, int, int, char *, char *, char **, int, dbref, dbref, char **, int, int));

extern INLINE int FDECL(safe_chr_real_fn, (char, char *, char **, int));
extern char *FDECL(upcasestr, (char *));
extern void FDECL(do_pemit_list, (dbref, char *, const char *, int));

#ifdef USE_COMSYS
extern void FDECL(make_cwho, (dbref, char *, char *, char **));
#endif

/* This is the prototype for functions */

#define	FUNCTION(x)	\
	void x(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs) \
	char *buff, **bufc; \
	dbref player, cause; \
	char *fargs[], *cargs[]; \
	int nfargs, ncargs;

#define Set_Max(x,y)     (x) = ((y) > (x)) ? (y) : (x);

/* --------------------------------------------------------------------------
 * Auxiliary functions for stacks.
 */

typedef struct object_stack STACK;
  struct object_stack {
      char *data;
      STACK *next;
  };

#define stack_get(x)   ((STACK *) nhashfind(x, &mudstate.objstack_htab))

#define stack_object(p,x)				\
        x = match_thing(p, fargs[0]);			\
	if (!Good_obj(x)) {				\
	    return;					\
	}						\
	if (!Controls(p, x)) {				\
            notify_quiet(p, NOPERM_MESSAGE);		\
	    return;					\
	}

/* --------------------------------------------------------------------------
 * Auxiliary stuff for structures.
 */

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

/* --------------------------------------------------------------------------
 * Auxiliary stuff for listing out hashtables.
 */

static void print_htab_matches(obj, htab, buff, bufc)
    dbref obj;
    HASHTAB *htab;
    char *buff;
    char **bufc;
{
    /* Things which use this are computationally expensive, and should
     * be discouraged.
     */

    char tbuf[SBUF_SIZE], *tp, *bb_p;
    HASHENT *hptr;
    int i, len;

    tp = tbuf;
    safe_ltos(tbuf, &tp, obj);
    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);

    bb_p = *bufc;

    for (i = 0; i < htab->hashsize; i++) {
	for (hptr = htab->entry->element[i]; hptr != NULL; hptr = hptr->next) {
	    if (!strncmp(tbuf, hptr->target, len)) {
		if (*bufc != bb_p)
		    safe_chr(' ', buff, bufc);
		safe_str((char *) (index(hptr->target, '.') + 1), buff, bufc);
	    }
	}
    }
}

/* --------------------------------------------------------------------------
 * Main body of functions starts here.
 */
	
#ifdef USE_COMSYS
FUNCTION(fun_cwho)
{
    make_cwho(player, fargs[0], buff, bufc);
}
#endif

FUNCTION(fun_beep)
{
	safe_chr(BEEP_CHAR, buff, bufc);
}

/* This function was originally taken from PennMUSH 1.50 */

FUNCTION(fun_ansi)
{
	char *s, *bb_p;

	if (!mudconf.ansi_colors) {
	    safe_str(fargs[1], buff, bufc);
	    return;
	}

	if (!fargs[0] || !*fargs[0]) {
	    safe_str(fargs[1], buff, bufc);
	    return;
	}

	/* Favor truncating the string over truncating the ANSI codes,
	 * but make sure to leave room for ANSI_NORMAL (4 characters).
	 * That means that we need a minimum of 9 (maybe 10) characters
	 * left in the buffer (8 or 9 in ANSI codes, plus at least one
	 * character worth of string to hilite). We do 10 just to be safe.
	 *
	 * This means that in MOST cases, we are not going to drop the
	 * trailing ANSI code for lack of space in the buffer. However,
	 * because of the possibility of an extended buffer created by
	 * exec(), this is not a guarantee (because extending the buffer
	 * gives us a fresh new buff, rather than having us continue to
	 * copy through the new buffer). Sadly, the times when we extend
	 * are also to be the times we're most likely to run out of space
	 * in the buffer. There's nothing we can do about that, though.
	 */

	if (strlen(buff) > LBUF_SIZE - 11)
	    return;

	s = fargs[0];
	safe_known_str(ANSI_BEGIN, 2, buff, bufc);
	bb_p = *bufc;

	while (*s) {
	    if (*bufc != bb_p) {
		safe_copy_chr(';', buff, bufc, LBUF_SIZE - 5);
	    }
	    switch (*s) {
		case 'h':	/* hilite */
		    safe_copy_str(N_ANSI_HILITE, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'i':	/* inverse */
		    safe_copy_str(N_ANSI_INVERSE, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'f':	/* flash */
		    safe_copy_str(N_ANSI_BLINK, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'u':	/* underline */
		    safe_copy_str(N_ANSI_UNDER, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'n':	/* normal */
		    safe_copy_str(N_ANSI_NORMAL, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'x':	/* black fg */
		    safe_copy_str(N_ANSI_BLACK, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'r':	/* red fg */
		    safe_copy_str(N_ANSI_RED, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'g':	/* green fg */
		    safe_copy_str(N_ANSI_GREEN, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'y':	/* yellow fg */
		    safe_copy_str(N_ANSI_YELLOW, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'b':	/* blue fg */
		    safe_copy_str(N_ANSI_BLUE, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'm':	/* magenta fg */
		    safe_copy_str(N_ANSI_MAGENTA, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'c':	/* cyan fg */
		    safe_copy_str(N_ANSI_CYAN, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'w':	/* white fg */
		    safe_copy_str(N_ANSI_WHITE, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'X':	/* black bg */
		    safe_copy_str(N_ANSI_BBLACK, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'R':	/* red bg */
		    safe_copy_str(N_ANSI_BRED, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'G':	/* green bg */
		    safe_copy_str(N_ANSI_BGREEN, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'Y':	/* yellow bg */
		    safe_copy_str(N_ANSI_BYELLOW, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'B':	/* blue bg */
		    safe_copy_str(N_ANSI_BBLUE, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'M':	/* magenta bg */
		    safe_copy_str(N_ANSI_BMAGENTA, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'C':	/* cyan bg */
		    safe_copy_str(N_ANSI_BCYAN, buff, bufc, LBUF_SIZE - 5);
		    break;
		case 'W':	/* white bg */
		    safe_copy_str(N_ANSI_BWHITE, buff, bufc, LBUF_SIZE - 5);
		    break;
	    }
	    s++;
	}
	safe_copy_chr(ANSI_END, buff, bufc, LBUF_SIZE - 5);
	safe_copy_str(fargs[1], buff, bufc, LBUF_SIZE - 5);
	safe_ansi_normal(buff, bufc);
}

FUNCTION(fun_zone)
{
	dbref it;

	if (!mudconf.have_zones) {
		return;
	}
	it = match_thing(player, fargs[0]);
	if (it == NOTHING || !Examinable(player, it)) {
		safe_nothing(buff, bufc);
		return;
	}
	safe_dbref(buff, bufc, Zone(it));
}

/* ---------------------------------------------------------------------------
 * Structures.
 */

static int istype_char(str)
    char *str;
{
    if (strlen(str) == 1)
	return 1;
    else
	return 0;
}

static int istype_dbref(str)
    char *str;
{
    dbref it;

    if (*str++ != NUMBER_TOKEN)
	return 0;
    if (*str) {
	it = parse_dbref(str);
	return (Good_obj(it));
    }
    return 0;
}

static int istype_int(str)
    char *str;
{
    return (is_integer(str));
}

static int istype_float(str)
    char *str;
{
    return (is_number(str));
}

static int istype_string(str)
    char *str;
{
    char *p;

    for (p = str; *p; p++) {
	if (isspace(*p))
	    return 0;
    }
    return 1;
}


FUNCTION(fun_structure)
{
    char sep;			/* delim for default values */
    char osep;			/* output delim for structure values */
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    char *comp_names, *type_names, *default_vals;
    char *comp_array[LBUF_SIZE / 2];
    char *type_array[LBUF_SIZE / 2];
    char *def_array[LBUF_SIZE / 2];
    int n_comps, n_types, n_defs;
    int i;
    STRUCTDEF *this_struct;
    COMPONENT *this_comp;
    int check_type = 0;

    svarargs_preamble("STRUCTURE", 6);

    /* Prevent null delimiters and line delimiters. */

    if (!osep) {
	notify_quiet(player, "You cannot use a null output delimiter.");
	safe_chr('0', buff, bufc);
	return;
    }
    if (osep == '\r') {
	notify_quiet(player, "You cannot use that output delimiter.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Enforce limits. */

    if (StructCount(player) > mudconf.struct_lim) {
	notify_quiet(player, "Too many structures.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* If our structure name is too long, reject it. */

    if (strlen(fargs[0]) > (SBUF_SIZE / 2) - 9) {
	notify_quiet(player, "Structure name is too long.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* No periods in structure names */

    if (((char *) index(fargs[0], '.')) != NULL) {
	notify_quiet(player, "Structure names cannot contain periods.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* The hashtable is indexed by <dbref number>.<structure name> */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], tbuf, &tp);
    *tp = '\0';

    /* If we have this structure already, reject. */

    if (hashfind(tbuf, &mudstate.structs_htab)) {
	notify_quiet(player, "Structure is already defined.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Split things up. Make sure lists are the same size.
     * If everything eventually goes well, comp_names and default_vals will
     * REMAIN allocated. 
     */


    comp_names = (char *) strdup(fargs[1]);
    n_comps = list2arr(comp_array, LBUF_SIZE / 2, comp_names, ' ');

    if (n_comps < 1) {
	notify_quiet(player, "There must be at least one component.");
	safe_chr('0', buff, bufc);
	free(comp_names);
	return;
    }

    /* Make sure that we have a sane name for the components. They must
     * be smaller than half an SBUF.
     */

    for (i = 0; i < n_comps; i++) {
	if (strlen(comp_array[i]) > (SBUF_SIZE / 2) - 9) {
	    notify_quiet(player, "Component name is too long.");
	    safe_chr('0', buff, bufc);
	    free(comp_names);
	    return;
	}
    }

    type_names = alloc_lbuf("struct.types");
    strcpy(type_names, fargs[2]);
    n_types = list2arr(type_array, LBUF_SIZE / 2, type_names, ' ');

    /* Make sure all types are valid. We look only at the first char, so
     * typos will not be caught.
     */

    for (i = 0; i < n_types; i++) {
	switch (*(type_array[i])) {
	    case 'a': case 'A':
	    case 'c': case 'C':
	    case 'd': case 'D':
	    case 'i': case 'I':
	    case 'f': case 'F':
	    case 's': case 'S':
		/* Valid types */
		break;
	    default:
		notify_quiet(player, "Invalid data type specified.");
		safe_chr('0', buff, bufc);
		free(comp_names);
		free_lbuf(type_names);
		return;
	}
    }

    if (fargs[3] && *fargs[3]) {
	default_vals = (char *) strdup(fargs[3]);
	n_defs = list2arr(def_array, LBUF_SIZE / 2, default_vals, sep);
    } else {
	default_vals = NULL;
	n_defs = 0;
    }

    if ((n_comps != n_types) || (n_defs && (n_comps != n_defs))) {
	notify_quiet(player, "List sizes must be identical.");
	safe_chr('0', buff, bufc);
	free(comp_names);
	free_lbuf(type_names);
	if (default_vals)
	    free(default_vals);
	return;
    }

    /* Allocate the structure and stuff it in the hashtable. Note that
     * we must duplicate our name structure since the pointers to the
     * strings have been allocated on the stack!
     */

    this_struct = (STRUCTDEF *) XMALLOC(sizeof(STRUCTDEF), "struct_alloc");
    this_struct->s_name = (char *) strdup(fargs[0]);
    this_struct->c_names = (char **) calloc(n_comps, sizeof(char *));
    for (i = 0; i < n_comps; i++)
	this_struct->c_names[i] = comp_array[i];
    this_struct->c_array = (COMPONENT **) calloc(n_comps, sizeof(COMPONENT *));
    this_struct->c_count = n_comps;
    this_struct->delim = osep;
    this_struct->n_instances = 0;
    this_struct->names_base = comp_names;
    this_struct->defs_base = default_vals;
    hashadd(tbuf, (int *) this_struct, &mudstate.structs_htab);
    Set_Max(mudstate.max_structs, mudstate.structs_htab.entries);

    /* Now that we're done with the base name, we can stick the 
     * joining period on the end.
     */
    
    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';

    /* Allocate each individual component. */

    for (i = 0; i < n_comps; i++) {

	cp = cbuf;
	safe_sb_str(tbuf, cbuf, &cp);
	for (p = comp_array[i]; *p; p++)
	    *p = ToLower(*p);
	safe_sb_str(comp_array[i], cbuf, &cp);
	*cp = '\0';

	this_comp = (COMPONENT *) XMALLOC(sizeof(COMPONENT), "comp_alloc");
	this_comp->def_val = def_array[i];
	switch (*(type_array[i])) {
	    case 'a': case 'A':
		this_comp->typer_func = NULL;
		break;
	    case 'c': case 'C':
		this_comp->typer_func = istype_char;
		check_type = 1;
		break;
	    case 'd': case 'D':
		this_comp->typer_func = istype_dbref;
		check_type = 1;
		break;
	    case 'i': case 'I':
		this_comp->typer_func = istype_int;
		check_type = 1;
		break;
	    case 'f': case 'F':
		this_comp->typer_func = istype_float;
		check_type = 1;
		break;
	    case 's': case 'S':
		this_comp->typer_func = istype_string;
		check_type = 1;
		break;
	    default:
		/* Should never happen */
		this_comp->typer_func = NULL;
	}
	this_struct->need_typecheck = check_type;
	this_struct->c_array[i] = this_comp;
	hashadd(cbuf, (int *) this_comp, &mudstate.cdefs_htab);
	Set_Max(mudstate.max_cdefs, mudstate.cdefs_htab.entries);
    }

    free_lbuf(type_names);
    s_StructCount(player, StructCount(player) + 1);
    safe_chr('1', buff, bufc);
}


FUNCTION(fun_construct)
{
    char sep;
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    STRUCTDEF *this_struct;
    char *comp_names, *init_vals;
    char *comp_array[LBUF_SIZE / 2], *vals_array[LBUF_SIZE / 2];
    int n_comps, n_vals;
    int i;
    COMPONENT *c_ptr;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int retval;

    /* This one is complicated: We need two, four, or five args. */

    mvarargs_preamble("CONSTRUCT", 2, 5);
    if (nfargs == 3) {
	safe_str("#-1 FUNCTION (CONSTRUCT) EXPECTS 2 OR 4 OR 5 ARGUMENTS",
		 buff, bufc);
	return;
    }

    /* Enforce limits. */

    if (InstanceCount(player) > mudconf.instance_lim) {
	notify_quiet(player, "Too many instances.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* If our instance name is too long, reject it. */

    if (strlen(fargs[0]) > (SBUF_SIZE / 2) - 9) {
	notify_quiet(player, "Instance name is too long.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Make sure this instance doesn't exist. */

    ip = ibuf;
    safe_ltos(ibuf, &ip, player);
    safe_sb_chr('.', ibuf, &ip);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mudstate.instance_htab)) {
	notify_quiet(player, "That instance has already been defined.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Look up the structure. */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[1]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[1], tbuf, &tp);
    *tp = '\0';

    this_struct = (STRUCTDEF *) hashfind(tbuf, &mudstate.structs_htab);
    if (!this_struct) {
	notify_quiet(player, "No such structure.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Check to make sure that all the component names are valid, if we
     * have been given defaults. Also, make sure that the defaults are
     * of the appropriate type.
     */

    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';

    if (fargs[2] && *fargs[2] && fargs[3] && *fargs[3]) {

	comp_names = alloc_lbuf("construct.comps");
	strcpy(comp_names, fargs[2]);
	n_comps = list2arr(comp_array, LBUF_SIZE / 2, comp_names, ' ');
	init_vals = alloc_lbuf("construct.vals");
	strcpy(init_vals, fargs[3]);
	n_vals = list2arr(vals_array, LBUF_SIZE / 2, init_vals, sep);
	if (n_comps != n_vals) {
	    notify_quiet(player, "List sizes must be identical.");
	    safe_chr('0', buff, bufc);
	    free_lbuf(comp_names);
	    free_lbuf(init_vals);
	    return;
	}

	for (i = 0; i < n_comps; i++) {
	    cp = cbuf;
	    safe_sb_str(tbuf, cbuf, &cp);
	    for (p = comp_array[i]; *p; p++)
		*p = ToLower(*p);
	    safe_sb_str(comp_array[i], cbuf, &cp);
	    c_ptr = (COMPONENT *) hashfind(cbuf, &mudstate.cdefs_htab);
	    if (!c_ptr) {
		notify_quiet(player, "Invalid component name.");
		safe_chr('0', buff, bufc);
		free_lbuf(comp_names);
		free_lbuf(init_vals);
		return;
	    }
	    if (c_ptr->typer_func) {
		retval = (*(c_ptr->typer_func)) (vals_array[i]);
		if (!retval) {
		    notify_quiet(player, "Default value is of invalid type.");
		    safe_chr('0', buff, bufc);
		    free_lbuf(comp_names);
		    free_lbuf(init_vals);
		    return;
		}
	    }
	}

    } else if ((!fargs[2] || !*fargs[2]) && (!fargs[3] || !*fargs[3])) {
	/* Blank initializers. This is just fine. */
	comp_names = init_vals = NULL;
	n_comps = n_vals = 0;
    } else {
	notify_quiet(player, "List sizes must be identical.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Go go gadget constructor.
     * Allocate the instance. We should have already made sure that the
     * instance doesn't exist.
     */

    inst_ptr = (INSTANCE *) XMALLOC(sizeof(INSTANCE), "constructor.inst");
    inst_ptr->datatype = this_struct;
    hashadd(ibuf, (int *) inst_ptr, &mudstate.instance_htab);
    Set_Max(mudstate.max_instance, mudstate.instance_htab.entries);

    /* Populate with default values. */

    for (i = 0; i < this_struct->c_count; i++) {
	d_ptr = (STRUCTDATA *) XMALLOC(sizeof(STRUCTDATA), "constructor.data");
	if (this_struct->c_array[i]->def_val) {
	    d_ptr->text = (char *)
		strdup(this_struct->c_array[i]->def_val);
	} else {
	    d_ptr->text = NULL;
	}
	tp = tbuf;
	safe_sb_str(ibuf, tbuf, &tp);
	safe_sb_chr('.', tbuf, &tp);
	safe_sb_str(this_struct->c_names[i], tbuf, &tp);
	*tp = '\0';
	hashadd(tbuf, (int *) d_ptr, &mudstate.instdata_htab);
	Set_Max(mudstate.max_instdata, mudstate.instdata_htab.entries);
    }

    /* Overwrite with component values. */

    for (i = 0; i < n_comps; i++) {
	tp = tbuf;
	safe_sb_str(ibuf, tbuf, &tp);
	safe_sb_chr('.', tbuf, &tp);
	safe_sb_str(comp_array[i], tbuf, &tp);
	*tp = '\0';
	d_ptr = (STRUCTDATA *) hashfind(tbuf, &mudstate.instdata_htab);
	if (d_ptr) {
	    if (d_ptr->text)
		free(d_ptr->text);
	    if (vals_array[i] && *(vals_array[i]))
		d_ptr->text = (char *) strdup(vals_array[i]);
	    else
		d_ptr->text = NULL;
	}
    }

    if (comp_names)
	free_lbuf(comp_names);
    if (init_vals)
	free_lbuf(init_vals);
    this_struct->n_instances += 1;
    s_InstanceCount(player, InstanceCount(player) + 1);
    safe_chr('1', buff, bufc);
}


FUNCTION(fun_load)
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    char *p;
    STRUCTDEF *this_struct;
    char *val_list;
    char *val_array[LBUF_SIZE / 2];
    int n_vals;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int i;
    char sep;

    varargs_preamble("LOAD", 4);

    /* Enforce limits. */

    if (InstanceCount(player) > mudconf.instance_lim) {
	notify_quiet(player, "Too many instances.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* If our instance name is too long, reject it. */

    if (strlen(fargs[0]) > (SBUF_SIZE / 2) - 9) {
	notify_quiet(player, "Instance name is too long.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Make sure this instance doesn't exist. */

    ip = ibuf;
    safe_ltos(ibuf, &ip, player);
    safe_sb_chr('.', ibuf, &ip);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mudstate.instance_htab)) {
	notify_quiet(player, "That instance has already been defined.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Look up the structure. */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[1]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[1], tbuf, &tp);
    *tp = '\0';

    this_struct = (STRUCTDEF *) hashfind(tbuf, &mudstate.structs_htab);
    if (!this_struct) {
	notify_quiet(player, "No such structure.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Chop up the raw stuff according to the delimiter. */

    if (nfargs != 4)
	sep = this_struct->delim;

    val_list = alloc_lbuf("load.val_list");
    strcpy(val_list, fargs[2]);
    n_vals = list2arr(val_array, LBUF_SIZE / 2, val_list, sep);
    if (n_vals != this_struct->c_count) {
	notify_quiet(player, "Incorrect number of components.");
	safe_chr('0', buff, bufc);
	free_lbuf(val_list);
	return;
    }

    /* Check the types of the data we've been passed. */

    for (i = 0; i < n_vals; i++) {
	if (this_struct->c_array[i]->typer_func &&
	    !((*(this_struct->c_array[i]->typer_func)) (val_array[i]))) {
	    notify_quiet(player, "Value is of invalid type.");
	    safe_chr('0', buff, bufc);
	    free_lbuf(val_list);
	    return;
	}
    }

    /* Allocate the instance. We should have already made sure that the
     * instance doesn't exist.
     */

    inst_ptr = (INSTANCE *) XMALLOC(sizeof(INSTANCE), "constructor.inst");
    inst_ptr->datatype = this_struct;
    hashadd(ibuf, (int *) inst_ptr, &mudstate.instance_htab);
    Set_Max(mudstate.max_instance, mudstate.instance_htab.entries);

    /* Stuff data into memory. */

    for (i = 0; i < this_struct->c_count; i++) {
	d_ptr = (STRUCTDATA *) XMALLOC(sizeof(STRUCTDATA), "constructor.data");
	if (val_array[i] && *(val_array[i]))
	    d_ptr->text = (char *) strdup(val_array[i]);
	else
	    d_ptr->text = NULL;
	tp = tbuf;
	safe_sb_str(ibuf, tbuf, &tp);
	safe_sb_chr('.', tbuf, &tp);
	safe_sb_str(this_struct->c_names[i], tbuf, &tp);
	*tp = '\0';
	hashadd(tbuf, (int *) d_ptr, &mudstate.instdata_htab);
	Set_Max(mudstate.max_instdata, mudstate.instdata_htab.entries);
    }

    free_lbuf(val_list);
    this_struct->n_instances += 1;
    s_InstanceCount(player, InstanceCount(player) + 1);
    safe_chr('1', buff, bufc);
}


FUNCTION(fun_z)
{
    char tbuf[SBUF_SIZE], *tp;
    char *p;
    STRUCTDATA *s_ptr;

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], tbuf, &tp);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[1]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[1], tbuf, &tp);
    *tp = '\0';

    s_ptr = (STRUCTDATA *) hashfind(tbuf, &mudstate.instdata_htab);
    if (!s_ptr || !s_ptr->text)
	return;
    safe_str(s_ptr->text, buff, bufc);
}


FUNCTION(fun_modify)
{
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    INSTANCE *inst_ptr;
    COMPONENT *c_ptr;
    STRUCTDATA *s_ptr;
    int retval;

    /* Find the instance first, since this is how we get our typechecker. */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], tbuf, &tp);
    *tp = '\0';

    inst_ptr = (INSTANCE *) hashfind(tbuf, &mudstate.instance_htab);
    if (!inst_ptr) {
	notify_quiet(player, "No such instance.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Use that to find the component and check the type. */

    if (inst_ptr->datatype->need_typecheck) {

	cp = cbuf;
	safe_ltos(cbuf, &cp, player);
	safe_sb_chr('.', cbuf, &cp);
	safe_sb_str(inst_ptr->datatype->s_name, cbuf, &cp);
	safe_sb_chr('.', cbuf, &cp);
	for (p = fargs[1]; *p; p++)
	    *p = ToLower(*p);
	safe_sb_str(fargs[1], cbuf, &cp);
	*cp = '\0';

	c_ptr = (COMPONENT *) hashfind(cbuf, &mudstate.cdefs_htab);
	if (!c_ptr) {
	    notify_quiet(player, "No such component.");
	    safe_chr('0', buff, bufc);
	    return;
	}
	if (c_ptr->typer_func) {
	    retval = (*(c_ptr->typer_func)) (fargs[2]);
	    if (!retval) {
		notify_quiet(player, "Value is of invalid type.");
		safe_chr('0', buff, bufc);
		return;
	    }
	}
    }

    /* Now go set it. */

    safe_sb_chr('.', tbuf, &tp);
    safe_sb_str(fargs[1], tbuf, &tp);
    *tp = '\0';

    s_ptr = (STRUCTDATA *) hashfind(tbuf, &mudstate.instdata_htab);
    if (!s_ptr) {
	notify_quiet(player, "No such data.");
	safe_chr('0', buff, bufc);
	return;
    }
    if (s_ptr->text)
	free(s_ptr->text);
    if (fargs[2] && *fargs[2]) {
	s_ptr->text = (char *) strdup(fargs[2]);
    } else {
	s_ptr->text = NULL;
    }

    safe_chr('1', buff, bufc);
}


FUNCTION(fun_unload)
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    INSTANCE *inst_ptr;
    char *p;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;
    char sep;

    varargs_preamble("UNLOAD", 2);

    /* Get the instance. */

    ip = ibuf;
    safe_ltos(ibuf, &ip, player);
    safe_sb_chr('.', ibuf, &ip);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], ibuf, &ip);
    *ip = '\0';

    inst_ptr = (INSTANCE *) hashfind(ibuf, &mudstate.instance_htab);
    if (!inst_ptr)
	return;

    /* From the instance, we can get a pointer to the structure. We then
     * have the information we need to figure out what components are
     * associated with this, and print them appropriately.
     */

    safe_sb_chr('.', ibuf, &ip);
    *ip = '\0';

    this_struct = inst_ptr->datatype;

    /* Our delimiter is a special case. */
    if (nfargs != 2)
	sep = this_struct->delim;

    for (i = 0; i < this_struct->c_count; i++) {
	if (i != 0) {
	    safe_chr(sep, buff, bufc);
	}
	tp = tbuf;
	safe_sb_str(ibuf, tbuf, &tp);
	safe_sb_str(this_struct->c_names[i], tbuf, &tp);
	*tp = '\0';
	d_ptr = (STRUCTDATA *) hashfind(tbuf, &mudstate.instdata_htab);
	if (d_ptr && d_ptr->text)
	    safe_str(d_ptr->text, buff, bufc);
    }
}


FUNCTION(fun_destruct)
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    INSTANCE *inst_ptr;
    char *p;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;

    /* Get the instance. */

    ip = ibuf;
    safe_ltos(ibuf, &ip, player);
    safe_sb_chr('.', ibuf, &ip);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], ibuf, &ip);
    *ip = '\0';

    inst_ptr = (INSTANCE *) hashfind(ibuf, &mudstate.instance_htab);
    if (!inst_ptr) {
	notify_quiet(player, "No such instance.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Now we can get a pointer to the structure and find the rest of the
     * components.
     */

    this_struct = inst_ptr->datatype;

    XFREE(inst_ptr, "constructor.inst");
    hashdelete(ibuf, &mudstate.instance_htab);

    safe_sb_chr('.', ibuf, &ip);
    *ip = '\0';

    for (i = 0; i < this_struct->c_count; i++) {
	tp = tbuf;
	safe_sb_str(ibuf, tbuf, &tp);
	safe_sb_str(this_struct->c_names[i], tbuf, &tp);
	*tp = '\0';
	d_ptr = (STRUCTDATA *) hashfind(tbuf, &mudstate.instdata_htab);
	if (d_ptr) {
	    if (d_ptr->text)
		free(d_ptr->text);
	    XFREE(d_ptr, "constructor.data");
	    hashdelete(tbuf, &mudstate.instdata_htab);
	}
    }

    this_struct->n_instances -= 1;
    s_InstanceCount(player, InstanceCount(player) - 1);
    safe_chr('1', buff, bufc);
}


FUNCTION(fun_unstructure)
{
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    STRUCTDEF *this_struct;
    int i;

    /* Find the structure */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], tbuf, &tp);
    *tp = '\0';

    this_struct = (STRUCTDEF *) hashfind(tbuf, &mudstate.structs_htab);
    if (!this_struct) {
	notify_quiet(player, "No such structure.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Can't delete what's in use. */

    if (this_struct->n_instances > 0) {
	notify_quiet(player, "This structure is in use.");
	safe_chr('0', buff, bufc);
	return;
    }

    /* Wipe the structure from the hashtable. */

    hashdelete(tbuf, &mudstate.structs_htab);

    /* Wipe out every component definition. */

    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';


    for (i = 0; i < this_struct->c_count; i++) {
	cp = cbuf;
	safe_sb_str(tbuf, cbuf, &cp);
	safe_sb_str(this_struct->c_names[i], cbuf, &cp);
	*cp = '\0';
	if (this_struct->c_array[i]) {
	    XFREE(this_struct->c_array[i], "comp_alloc");
	}
	hashdelete(cbuf, &mudstate.cdefs_htab);
    }


    /* Free up our bit of memory. */

    free(this_struct->s_name);
    if (this_struct->names_base)
	free(this_struct->names_base);
    if (this_struct->defs_base)
	free(this_struct->defs_base);
    free(this_struct->c_names);
    XFREE(this_struct, "struct_alloc");

    s_StructCount(player, StructCount(player) - 1);
    safe_chr('1', buff, bufc);
}

FUNCTION(fun_lstructures)
{
    print_htab_matches(player, &mudstate.structs_htab, buff, bufc);
}

FUNCTION(fun_linstances)
{
    print_htab_matches(player, &mudstate.instance_htab, buff, bufc);
}

void structure_clr(thing)
    dbref thing;
{
    /* Wipe out all structure information associated with an object.
     * Find all the object's instances. Destroy them.
     * Then, find all the object's defined structures, and destroy those.
     */

    HASHTAB *htab;
    HASHENT *hptr;
    char tbuf[SBUF_SIZE], ibuf[SBUF_SIZE], cbuf[SBUF_SIZE], *tp, *ip, *cp;
    int i, j, len, count;
    INSTANCE **inst_array;
    char **name_array;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    STRUCTDEF **struct_array;

    /* The instance table is indexed as <dbref number>.<instance name> */

    tp = tbuf;
    safe_ltos(tbuf, &tp, thing);
    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);

    /* Because of the hashtable rechaining that's done, we cannot simply
     * walk the hashtable and delete entries as we go. Instead, we've
     * got to keep track of all of our pointers, and go back and do
     * them one by one.
     */

    inst_array = (INSTANCE **) calloc(mudconf.instance_lim + 1,
				      sizeof(INSTANCE *));
    name_array = (char **) calloc(mudconf.instance_lim + 1, sizeof(char *));
    
    htab = &mudstate.instance_htab;
    count = 0;
    for (i = 0; i < htab->hashsize; i++) {
	for (hptr = htab->entry->element[i]; hptr != NULL; hptr = hptr->next) {
	    if (!strncmp(tbuf, hptr->target, len)) {
		name_array[count] = (char *) hptr->target;
		inst_array[count] = (INSTANCE *) hptr->data;
		count++;
	    }
	}
    }

    /* Now that we have the pointers to the instances, we can get the
     * structure definitions, and use that to hunt down and wipe the
     * components. 
     */

    if (count > 0) {
	for (i = 0; i < count; i++) {
	    this_struct = inst_array[i]->datatype;
	    XFREE(inst_array[i], "constructor.inst");
	    hashdelete(name_array[i], &mudstate.instance_htab);
	    ip = ibuf;
	    safe_sb_str(name_array[i], ibuf, &ip);
	    safe_sb_chr('.', ibuf, &ip);
	    *ip = '\0';
	    for (j = 0; j < this_struct->c_count; j++) {
		cp = cbuf;
		safe_sb_str(ibuf, cbuf, &cp);
		safe_sb_str(this_struct->c_names[j], cbuf, &cp);
		*cp = '\0';
		d_ptr = (STRUCTDATA *) hashfind(cbuf, &mudstate.instdata_htab);
		if (d_ptr) {
		    if (d_ptr->text)
			free(d_ptr->text);
		    XFREE(d_ptr, "constructor.data");
		    hashdelete(cbuf, &mudstate.instdata_htab);
		}
	    }
	    this_struct->n_instances -= 1;
	}
    }

    free(inst_array);
    free(name_array);

    /* The structure table is indexed as <dbref number>.<struct name> */

    tp = tbuf;
    safe_ltos(tbuf, &tp, thing);
    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);

    /* Again, we have the hashtable rechaining problem. */

    struct_array = (STRUCTDEF **) calloc(mudconf.struct_lim + 1,
					 sizeof(STRUCTDEF *));
    name_array = (char **) calloc(mudconf.struct_lim + 1, sizeof(char *));

    htab = &mudstate.structs_htab;
    count = 0;
    for (i = 0; i < htab->hashsize; i++) {
	for (hptr = htab->entry->element[i]; hptr != NULL; hptr = hptr->next) {
	    if (!strncmp(tbuf, hptr->target, len)) {
		name_array[count] = (char *) hptr->target;
		struct_array[count] = (STRUCTDEF *) hptr->data;
		count++;
	    }
	}
    }

    /* We have the pointers to the structures. Flag a big error if they're
     * still in use, wipe them from the hashtable, then wipe out every
     * component definition. Free up the memory.
     */

    if (count > 0) {
	for (i = 0; i < count; i++) {
	    if (struct_array[i]->n_instances > 0) {
		STARTLOG(LOG_ALWAYS, "BUG", "STRUCT")
		    log_name(thing);
		    log_text((char *) " structure ");
		    log_text((char *) name_array[i]);
		    log_text((char *) " has ");
		    log_number(struct_array[i]->n_instances);
		    log_text((char *) " allocated instances uncleared.");
		ENDLOG
	    }
	    hashdelete(name_array[i], &mudstate.structs_htab);
	    ip = ibuf;
	    safe_sb_str(name_array[i], ibuf, &ip);
	    safe_sb_chr('.', ibuf, &ip);
	    *ip = '\0';
	    for (j = 0; j < struct_array[i]->c_count; j++) {
		cp = cbuf;
		safe_sb_str(ibuf, cbuf, &cp);
		safe_sb_str(struct_array[i]->c_names[j], cbuf, &cp);
		*cp = '\0';
		if (struct_array[i]->c_array[j]) {
		    XFREE(struct_array[i]->c_array[j], "comp_alloc");
		}
		hashdelete(cbuf, &mudstate.cdefs_htab);
	    }
	    free(struct_array[i]->s_name);
	    if (struct_array[i]->names_base)
		free(struct_array[i]->names_base);
	    if (struct_array[i]->defs_base)
		free(struct_array[i]->defs_base);
	    free(struct_array[i]->c_names);
	    XFREE(struct_array[i], "struct_alloc");
	}
    }

    free(struct_array);
    free(name_array);
}


/*------------------------------------------------------------------------
 * Side-effect functions.
 */

static int check_command(player, name, buff, bufc)
dbref player;
char *name, *buff, **bufc;
{
    CMDENT *cmdp;

    if ((cmdp = (CMDENT *) hashfind(name, &mudstate.command_htab))) {

	/* Note that these permission checks are NOT identical to the
	 * ones in process_cmdent(). In particular, side-effects are NOT
	 * subject to the CA_GBL_INTERP flag. This is a design decision
	 * based on the concept that these are functions and not commands,
	 * even though they behave like commands in many respects. This
	 * is also the same reason why side-effects don't trigger hooks.
	 */

	if (Invalid_Objtype(player) || !check_access(player, cmdp->perms) ||
	    (!Builder(player) && Protect(CA_GBL_BUILD) &&
	     !(mudconf.control_flags & CF_BUILD))) {

	    safe_noperm(buff, bufc);
	    return 1;
	}
    }

    return 0;
}


FUNCTION(fun_link)
{
    if (check_command(player, "@link", buff, bufc))
	return;
    do_link(player, cause, 0, fargs[0], fargs[1]);
}

FUNCTION(fun_tel)
{
    if (check_command(player, "@teleport", buff, bufc))
	return;
    do_teleport(player, cause, 0, fargs[0], fargs[1]);
}

FUNCTION(fun_wipe)
{
    if (check_command(player, "@wipe", buff, bufc))
	return;
    do_wipe(player, cause, 0, fargs[0]);
}

FUNCTION(fun_pemit)
{
    if (check_command(player, "@pemit", buff, bufc))
	return;
    do_pemit_list(player, fargs[0], fargs[1], 0);
}

FUNCTION(fun_remit)
{
    if (check_command(player, "@pemit", buff, bufc))
	return;
    do_pemit_list(player, fargs[0], fargs[1], 1);
}

FUNCTION(fun_force)
{
    if (check_command(player, "@force", buff, bufc))
	return;
    do_force(player, cause, 0, fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_trigger)
{
	if (nfargs < 1) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	if (check_command(player, "@trigger", buff, bufc))
	    return;
	do_trigger(player, cause, 0, fargs[0], &(fargs[1]), nfargs - 1);
}

FUNCTION(fun_wait)
{
    do_wait(player, cause, 0, fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_command)
{
    CMDENT *cmdp;
    char tbuf1[1], tbuf2[1];
    int key;

    if (!fargs[0] || !*fargs[0])
	return;

    cmdp = (CMDENT *) hashfind(fargs[0], &mudstate.command_htab);
    if (!cmdp) {
	notify(player, "Command not found.");
	return;
    }

    if (Invalid_Objtype(player) || !check_access(player, cmdp->perms) ||
	(!Builder(player) && Protect(CA_GBL_BUILD) &&
	 !(mudconf.control_flags & CF_BUILD))) {
	notify(player, "Permission denied.");
	return;
    }

    if (!(cmdp->callseq & CS_FUNCTION) || (cmdp->callseq & CS_ADDED)) {
	notify(player, "Cannot call that command.");
	return;
    }

    /* Strip command flags that are irrelevant. */

    key = cmdp->extra;
    key &= ~(SW_GOT_UNIQUE | SW_MULTIPLE | SW_NOEVAL);

    /* Can't handle null args, so make sure there's something there. */

    tbuf1[0] = '\0';
    tbuf2[0] = '\0';

    switch (cmdp->callseq & CS_NARG_MASK) {
	case CS_NO_ARGS:
	    (*(cmdp->info.handler)) (player, cause, key);
	    break;
	case CS_ONE_ARG:
	    (*(cmdp->info.handler)) (player, cause, key,
				     ((fargs[1]) ? (fargs[1]) : tbuf1));
	    break;
	case CS_TWO_ARG:
	    (*(cmdp->info.handler)) (player, cause, key,
				     ((fargs[1]) ? (fargs[1]) : tbuf1),
				     ((fargs[2]) ? (fargs[2]) : tbuf2));
	    break;
	default:
	    notify(player, "Invalid command handler.");
	    return;
    }
}


/*------------------------------------------------------------------------
 * fun_create: Creates a room, thing or exit
 */

FUNCTION(fun_create)
{
	dbref thing;
	int cost;
	char sep, *name;

	varargs_preamble("CREATE", 3);
	name = fargs[0];

	if (!name || !*name) {
		safe_str("#-1 ILLEGAL NAME", buff, bufc);
		return;
	}
	if (fargs[2] && *fargs[2])
		sep = *fargs[2];
	else
		sep = 't';

	switch (sep) {
	case 'r':
		if (check_command(player, "@dig", buff, bufc)) {
			return;
		}
		thing = create_obj(player, TYPE_ROOM, name, 0);
		break;
	case 'e':
		if (check_command(player, "@open", buff, bufc)) {
			return;
		}
		thing = create_obj(player, TYPE_EXIT, name, 0);
		if (thing != NOTHING) {
			s_Exits(thing, player);
			s_Next(thing, Exits(player));
			s_Exits(player, thing);
		}
		break;
	default:
		if (check_command(player, "@create", buff, bufc)) {
			return;
		}
		if (fargs[1] && *fargs[1]) {
		    cost = atoi(fargs[1]);
		    if (cost < mudconf.createmin || cost > mudconf.createmax) {
			safe_str("#-1 COST OUT OF RANGE", buff, bufc);
			return;
		    }
		} else {
		    cost = mudconf.createmin;
		}
		thing = create_obj(player, TYPE_THING, name, cost);
		if (thing != NOTHING) {
			move_via_generic(thing, player, NOTHING, 0);
			s_Home(thing, new_home(player));
		}
		break;
	}
	safe_dbref(buff, bufc, thing);
}

/*---------------------------------------------------------------------------
 * fun_set: sets an attribute on an object
 */

static void set_attr_internal(player, thing, attrnum, attrtext, key, buff, bufc)
dbref player, thing;
int attrnum, key;
char *attrtext, *buff;
char **bufc;
{
	dbref aowner;
	int aflags, could_hear;
	ATTR *attr;

	attr = atr_num(attrnum);
	atr_pget_info(thing, attrnum, &aowner, &aflags);
	if (attr && Set_attr(player, thing, attr, aflags)) {
		if ((attr->check != NULL) &&
		    (!(*attr->check) (0, player, thing, attrnum, attrtext))) {
		        safe_noperm(buff, bufc);
			return;
		}
		could_hear = Hearer(thing);
		atr_add(thing, attrnum, attrtext, Owner(player), aflags);
		handle_ears(thing, could_hear, Hearer(thing));
		if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing))
			notify_quiet(player, "Set.");
	} else {
		safe_str("#-1 PERMISSION DENIED.", buff, bufc);
	}
}

FUNCTION(fun_set)
{
	dbref thing, thing2, aowner;
	char *p, *buff2;
	int atr, atr2, aflags, alen, clear, flagvalue, could_hear;
	ATTR *attr, *attr2;

	/* obj/attr form? */

	if (check_command(player, "@set", buff, bufc))
	    return;

	if (parse_attrib(player, fargs[0], &thing, &atr)) {
		if (atr != NOTHING) {

			/* must specify flag name */

			if (!fargs[1] || !*fargs[1]) {

				safe_str("#-1 UNSPECIFIED PARAMETER", buff, bufc);
			}
			/* are we clearing? */

			clear = 0;
			if (*fargs[0] == NOT_TOKEN) {
				fargs[0]++;
				clear = 1;
			}
			/* valid attribute flag? */

			flagvalue = search_nametab(player,
					indiv_attraccess_nametab, fargs[1]);
			if (flagvalue < 0) {
				safe_str("#-1 CAN NOT SET", buff, bufc);
				return;
			}
			/* make sure attribute is present */

			if (!atr_get_info(thing, atr, &aowner, &aflags)) {
				safe_str("#-1 ATTRIBUTE NOT PRESENT ON OBJECT", buff, bufc);
				return;
			}
			/* can we write to attribute? */

			attr = atr_num(atr);
			if (!attr || !Set_attr(player, thing, attr, aflags)) {
				safe_noperm(buff, bufc);
				return;
			}
			/* just do it! */

			if (clear)
				aflags &= ~flagvalue;
			else
				aflags |= flagvalue;
			could_hear = Hearer(thing);
			atr_set_flags(thing, atr, aflags);

			return;
		}
	}
	/* find thing */

	if ((thing = match_controlled(player, fargs[0])) == NOTHING) {
		safe_nothing(buff, bufc);
		return;
	}
	/* check for attr set first */
	for (p = fargs[1]; *p && (*p != ':'); p++) ;

	if (*p) {
		*p++ = 0;
		atr = mkattr(fargs[1]);
		if (atr <= 0) {
			safe_str("#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc);
			return;
		}
		attr = atr_num(atr);
		if (!attr) {
			safe_noperm(buff, bufc);
			return;
		}
		atr_get_info(thing, atr, &aowner, &aflags);
		if (!Set_attr(player, thing, attr, aflags)) {
			safe_noperm(buff, bufc);
			return;
		}
		buff2 = alloc_lbuf("fun_set");

		/* check for _ */
		if (*p == '_') {
			strcpy(buff2, p + 1);
			if (!parse_attrib(player, p + 1, &thing2, &atr2) ||
			    (atr == NOTHING)) {
				free_lbuf(buff2);
				safe_nomatch(buff, bufc);
				return;
			}
			attr2 = atr_num(atr);
			p = buff2;
			atr_pget_str(buff2, thing2, atr2, &aowner,
				     &aflags, &alen);

			if (!attr2 ||
			 !See_attr(player, thing2, attr2, aowner, aflags)) {
				free_lbuf(buff2);
				safe_noperm(buff, bufc);
				return;
			}
		}
		/* set it */

		set_attr_internal(player, thing, atr, p, 0, buff, bufc);
		free_lbuf(buff2);
		return;
	}
	/* set/clear a flag */
	flag_set(thing, player, fargs[1], 0);
}

/*---------------------------------------------------------------------------
 */

/* Borrowed from DarkZone */
FUNCTION(fun_zwho)
{
	dbref i, it = match_thing(player, fargs[0]);
	int len = 0;
	char *smbuf;
	
	if (!mudconf.have_zones || (!Controls(player, it) && !WizRoy(player))) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}
	for (i = 0; i < mudstate.db_top; i++) {
	    if (Typeof(i) == TYPE_PLAYER) {
		if (Zone(i) == it) {
		    if (len) {
			smbuf = alloc_sbuf("fun_zwho");
			sprintf(smbuf, " #%d", i);
			if ((strlen(smbuf) + len) > 990) {
			    safe_known_str(" #-1", 4, buff, bufc);
			    free_sbuf(smbuf);
			    return;
			}
			safe_str(smbuf, buff, bufc);
			len += strlen(smbuf);
			free_sbuf(smbuf);
		    } else {
			safe_dbref(buff, bufc, i);
			len = strlen(buff);
		    }
		}
	    }
	}
}

/* Borrowed from DarkZone */
FUNCTION(fun_inzone)
{
	dbref i, it = match_thing(player, fargs[0]);
	int len = 0;
	char *smbuf;

	if (!mudconf.have_zones || (!Controls(player, it) && !WizRoy(player))) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}
	for (i = 0; i < mudstate.db_top; i++) {
	    if (Typeof(i) == TYPE_ROOM) {
		if (db[i].zone == it) {
		    if (len) {
			smbuf = alloc_sbuf("fun_inzone");;
			sprintf(smbuf, " #%d", i);
			if ((strlen(smbuf) + len) > 990) {
			    safe_known_str(" #-1", 4, buff, bufc);
			    free_sbuf(smbuf);
			    return;
			}
			safe_str(smbuf, buff, bufc);
			len += strlen(smbuf);
			free_sbuf(smbuf);
		    } else {
			safe_dbref(buff, bufc, i);
			len = strlen(buff);
		    }
		}
	    }
	}
}

/* Borrowed from DarkZone */
FUNCTION(fun_children)
{
	dbref i, it = match_thing(player, fargs[0]);
	int len = 0;
	char *smbuf;

	if (!(Controls(player, it)) || !(WizRoy(player))) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}
	for (i = 0; i < mudstate.db_top; i++) {
	    if (Parent(i) == it) {
		if (len) {
		    smbuf = alloc_sbuf("fun_children");
		    sprintf(smbuf, " #%d", i);
		    if ((strlen(smbuf) + len) > 990) {
			safe_known_str(" #-1", 4, buff, bufc);
			free_sbuf(smbuf);
			return;
		    }
		    safe_str(smbuf, buff, bufc);
		    len += strlen(smbuf);
		    free_sbuf(smbuf);
		} else {
		    safe_dbref(buff, bufc, i);
		    len = strlen(buff);
		}
	    }
	}
}

FUNCTION(fun_objeval)
{
	dbref obj;
	char *name, *bp, *str;

	if (!*fargs[0]) {
		return;
	}
	name = bp = alloc_lbuf("fun_objeval");
	str = fargs[0];
	exec(name, &bp, 0, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str,
	     cargs, ncargs);
	*bp = '\0';
	obj = match_thing(player, name);

	/* In order to evaluate from something else's viewpoint, you must
	 * have the same owner as it, or be a wizard. Otherwise, we default
	 * to evaluating from our own viewpoint. Also, you cannot evaluate
	 * things from the point of view of God.
	 */
	if ((obj == NOTHING) || (obj == GOD) ||
	    ((Owner(obj) != Owner(player)) && !Wizard(player))) {
		obj = player;
	}

	str = fargs[1];
	exec(buff, bufc, 0, obj, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str,
	     cargs, ncargs);
	free_lbuf(name);
}

/* ---------------------------------------------------------------------------
 * fun_localize: Evaluate a function with local scope (i.e., preserve and
 * restore the r-registers). Essentially like calling ulocal() but with the
 * function string directly.
 */

FUNCTION(fun_localize)
{
    char *str, *preserve[MAX_GLOBAL_REGS];
    int preserve_len[MAX_GLOBAL_REGS];

    save_global_regs("fun_localize_save", preserve, preserve_len);

    str = fargs[0];
    exec(buff, bufc, 0, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str,
	 cargs, ncargs);

    restore_global_regs("fun_localize_restore", preserve, preserve_len);
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
    char *tp, *bp, sep;

    if (nfargs == 0) {
	return;
    }

    varargs_preamble("SQUISH", 2);

    bp = tp = fargs[0];

    while (*tp) {

	/* Move over and copy the non-sep characters */

	while (*tp && (*tp != sep)) {
	    *bp++ = *tp++;
	}

	/* If we've reached the end of the string, leave the loop. */

	if (!*tp)
	    break;

	/* Otherwise, we've hit a sep char. Move over it, and then move on to
	 * the next non-separator. Note that we're overwriting our own
	 * string as we do this. However, the other pointer will always
	 * be ahead of our current copy pointer.
	 */

	*bp++ = *tp++;
	while (*tp && (*tp == sep))
	    tp++;
    }

    /* Must terminate the string */

    *bp = '\0';
    
    safe_str(fargs[0], buff, bufc);
}


FUNCTION(fun_stripansi)
{
	safe_str((char *)strip_ansi(fargs[0]), buff, bufc);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_zfun)
{
	dbref aowner;
	int aflags, alen;
	int attrib;
	char *tbuf1, *str;

	dbref zone = Zone(player);

	if (!mudconf.have_zones) {
		safe_str("#-1 ZONES DISABLED", buff, bufc);
		return;
	}
	if (zone == NOTHING) {
		safe_str("#-1 INVALID ZONE", buff, bufc);
		return;
	}
	if (!fargs[0] || !*fargs[0])
		return;

	/* find the user function attribute */
	attrib = get_atr(upcasestr(fargs[0]));
	if (!attrib) {
		safe_str("#-1 NO SUCH USER FUNCTION", buff, bufc);
		return;
	}
	tbuf1 = atr_pget(zone, attrib, &aowner, &aflags, &alen);
	if (!See_attr(player, zone, (ATTR *) atr_num(attrib), aowner, aflags)) {
		safe_str("#-1 NO PERMISSION TO GET ATTRIBUTE", buff, bufc);
		free_lbuf(tbuf1);
		return;
	}
	str = tbuf1;
	exec(buff, bufc, 0, zone, player, EV_EVAL | EV_STRIP | EV_FCHECK, &str, &(fargs[1]),
	     nfargs - 1);
	free_lbuf(tbuf1);
}

FUNCTION(fun_columns)
{
	unsigned int spaces, number, ansinumber, striplen;
	unsigned int count, i, indent = 0;
	int isansi = 0, rturn = 1, cr = 0;
	char *p, *q, *buf, *curr, *objstring, *bp, *cp, sep, *str;

        if (!fn_range_check("COLUMNS", nfargs, 2, 4, buff, bufc))
		return;
	if (!delim_check(fargs, nfargs, 3, &sep, buff, bufc, 1,                
	    player, cause, cargs, ncargs, 0))
		return;
		
	number = (unsigned int) safe_atoi(fargs[1]);
	indent = (unsigned int) safe_atoi(fargs[3]);

	if (indent > 77) {	/* unsigned int, always a positive number */
		indent = 1;
	}

	/* Must check number separately, since number + indent can
	 * result in an integer overflow.
	 */

	if ((number < 1) || (number > 77) ||
	    ((unsigned int) (number + indent) > 78)) {
		safe_str("#-1 OUT OF RANGE", buff, bufc);
		return;
	}

	cp = curr = bp = alloc_lbuf("fun_columns");
	str = fargs[0];
	exec(curr, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	     cargs, ncargs);
	*bp = '\0';
	cp = trim_space_sep(cp, sep);
	if (!*cp) {
		free_lbuf(curr);
		return;
	}
	
	for (i = 0; i < indent; i++)
		safe_chr(' ', buff, bufc);

	buf = alloc_lbuf("fun_columns");
	
	while (cp) {
		objstring = split_token(&cp, sep);
		ansinumber = number;
		striplen = strlen((char *) strip_ansi(objstring));
		if (ansinumber > striplen)
		    ansinumber = striplen;

		p = objstring;
		q = buf;
		count = 0;
		while (p && *p) {
			if (count == number) {
				break;
			}
			if (*p == ESC_CHAR) {
				/* Start of ANSI code. Skip to end. */
				isansi = 1;
				while (*p && !isalpha(*p))
					*q++ = *p++;
				if (*p)
					*q++ = *p++;
			} else {
				*q++ = *p++;
				count++;
			}
		}
		if (isansi)
		    safe_ansi_normal(buf, &q);
		*q = '\0';
		isansi = 0;

		safe_str(buf, buff, bufc);

		if (striplen < number) {

		    /* We only need spaces if we need to pad out.
		     * Sanitize the number of spaces, too. 
		     */

		    spaces = number - striplen;
		    if (spaces > LBUF_SIZE) {
			spaces = LBUF_SIZE;
		    }

		    for (i = 0; i < spaces; i++)
			safe_chr(' ', buff, bufc);
		}

		if (!(rturn % (int)((78 - indent) / number))) {
			safe_crlf(buff, bufc);
			cr = 1;
			for (i = 0; i < indent; i++)
				safe_chr(' ', buff, bufc);
		} else {
			cr = 0;
		}

		rturn++;
	}
	
	if (!cr) {
		safe_crlf(buff, bufc);
	}
	
	free_lbuf(buf);
	free_lbuf(curr);
}

/* Code for objmem and playmem borrowed from PennMUSH 1.50 */

static int mem_usage(thing)
dbref thing;
{
	int k;
	int ca;
	char *as, *str;
	ATTR *attr;

	k = sizeof(OBJ);

	k += strlen(Name(thing)) + 1;
	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as)) {
		str = atr_get_raw(thing, ca);
		if (str && *str)
			k += strlen(str);
		attr = atr_num(ca);
		if (attr) {
			str = (char *)attr->name;
			if (str && *str)
				k += strlen(((ATTR *) atr_num(ca))->name);
		}
	}
	return k;
}

static int mem_usage_attr(player, str)
    dbref player;
    char *str;
{
    dbref thing, aowner;
    int atr, aflags, alen;
    char *abuf;
    ATTR *ap;
    int bytes_atext = 0;

    olist_push();
    if (parse_attrib_wild(player, str, &thing, 0, 0, 1)) {
	for (atr = olist_first(); atr != NOTHING; atr = olist_next()) {
	    ap = atr_num(atr);
	    if (!ap)
		continue;
	    abuf = atr_get(thing, atr, &aowner, &aflags, &alen);
	    /* Player must be able to read attribute with 'examine' */
	    if (Examinable(player, thing) &&
		Read_attr(player, thing, ap, aowner, aflags))
		bytes_atext += strlen(abuf);
	    free_lbuf(abuf);
	}
    }

    olist_pop();
    return bytes_atext;
}

FUNCTION(fun_objmem)
{
	dbref thing;

	if (index(fargs[0], '/') != NULL) {
	    safe_ltos(buff, bufc, mem_usage_attr(player, fargs[0]));
	    return;
	}

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING || !Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	safe_ltos(buff, bufc, mem_usage(thing));
}

FUNCTION(fun_playmem)
{
	int tot = 0;
	dbref thing;
	dbref j;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING || !Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	DO_WHOLE_DB(j)
		if (Owner(j) == thing)
		tot += mem_usage(j);
	safe_ltos(buff, bufc, tot);
}

/* Code for andflags() and orflags() borrowed from PennMUSH 1.50 */
static int handle_flaglists(player, cause, name, fstr, type)
dbref player, cause;
char *name;
char *fstr;
int type;			/* 0 for orflags, 1 for andflags */
{
	char *s;
	char flagletter[2];
	FLAGSET fset;
	FLAG p_type;
	int negate, temp;
	int ret = type;
	dbref it = match_thing(player, name);

	negate = temp = 0;

	if (it == NOTHING)
		return 0;
	if (! (mudconf.pub_flags || Examinable(player, it) || (it == cause)))
		return 0;
		
	for (s = fstr; *s; s++) {

		/* Check for a negation sign. If we find it, we note it and 
		 * increment the pointer to the next character. 
		 */

		if (*s == '!') {
			negate = 1;
			s++;
		} else {
			negate = 0;
		}

		if (!*s) {
			return 0;
		}
		flagletter[0] = *s;
		flagletter[1] = '\0';

		if (!convert_flags(player, flagletter, &fset, &p_type)) {

			/* Either we got a '!' that wasn't followed by a
			 * letter, or we couldn't find that flag. For
			 * AND, since we've failed a check, we can
			 * return false. Otherwise we just go on. 
			 */

			if (type == 1)
				return 0;
			else
				continue;

		} else {

			/* does the object have this flag? */

			if ((Flags(it) & fset.word1) ||
			    (Flags2(it) & fset.word2) ||
			    (Flags3(it) & fset.word3) ||
			    (Typeof(it) == p_type)) {
				if (isPlayer(it) && (fset.word2 == CONNECTED)
				    && Hidden(it) && !See_Hidden(player))
					temp = 0;
				else
					temp = 1;
			} else {
				temp = 0;
			}
			
			if ((type == 1) && ((negate && temp) || (!negate && !temp))) {

				/* Too bad there's no NXOR function... At
				 * this point we've either got a flag
				 * and we don't want it, or we don't
				 * have a flag and we want it. Since
				 * it's AND, we return false. 
				 */
				return 0;

			} else if ((type == 0) &&
				 ((!negate && temp) || (negate && !temp))) {

				/* We've found something we want, in an OR. */

				return 1;
			}
			/* Otherwise, we don't need to do anything. */
		}
	}
	return (ret);
}

FUNCTION(fun_orflags)
{
	safe_ltos(buff, bufc, handle_flaglists(player, cause, fargs[0], fargs[1], 0));
}

FUNCTION(fun_andflags)
{
	safe_ltos(buff, bufc, handle_flaglists(player, cause, fargs[0], fargs[1], 1));
}

FUNCTION(fun_strtrunc)
{
	int number, count = 0;
	char *p = (char *)fargs[0];
	char *q, *buf;
	int isansi = 0;

	q = buf = alloc_lbuf("fun_strtrunc");
	number = atoi(fargs[1]);
	if (number > strlen((char *)strip_ansi(fargs[0])))
		number = strlen((char *)strip_ansi(fargs[0]));

	if (number < 0) {
		safe_str("#-1 OUT OF RANGE", buff, bufc);
		free_lbuf(buf);
		return;
	}
	while (p && *p) {
		if (count == number) {
			break;
		}
		if (*p == ESC_CHAR) {
			/* Start of ANSI code. Skip to end. */
			isansi = 1;
			while (*p && !isalpha(*p))
				*q++ = *p++;
			if (*p)
				*q++ = *p++;
		} else {
			*q++ = *p++;
			count++;
		}
	}
	if (isansi)
	    safe_ansi_normal(buf, &q);
	*q = '\0';
	safe_str(buf, buff, bufc);
	free_lbuf(buf);
}

FUNCTION(fun_ifelse)
{
	/* This function now assumes that its arguments have not been
	   evaluated. */
	
	char *str, *mbuff, *bp;
	
	mbuff = bp = alloc_lbuf("fun_ifelse");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		&str, cargs, ncargs);
	*bp = '\0';
	
	if (!mbuff || !*mbuff || !xlate(mbuff)) {
		str = fargs[2];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	} else {
		str = fargs[1];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	}
	free_lbuf(mbuff);
}

FUNCTION(fun_nonzero)
{
	/* MUX-style ifelse -- rather than bool check, check if the
	* string is non-null/non-zero.
	*/
	
	char *str, *mbuff, *bp;
	
	mbuff = bp = alloc_lbuf("fun_nonzero");
	str = fargs[0];
	exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		&str, cargs, ncargs);
	*bp = '\0';
	
	if (!mbuff || !*mbuff || ((atoi(mbuff) == 0) && is_number(mbuff))) {
		str = fargs[2];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	} else {
		str = fargs[1];
		exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
			&str, cargs, ncargs);
	}
	free_lbuf(mbuff);
}

FUNCTION(fun_inc)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) + 1);
}

FUNCTION(fun_dec)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) - 1);
}

#ifdef USE_MAIL
/* Mail functions borrowed from DarkZone */
FUNCTION(fun_mail)
{
	/* This function can take one of three formats: 1.  mail(num)  -->
	 * returns message <num> for privs. 2.  mail(player)  -->
	 * returns number of messages for <player>. 3.
	 * mail(player, num)  -->  returns message <num> for
	 * <player>. 
	 *
	 * It can now take one more format: 4.  mail() --> returns number of
	 * messages for executor 
	 */

	struct mail *mp;
	dbref playerask;
	int num, rc, uc, cc;
#ifdef RADIX_COMPRESSION
	char *msgbuff;
#endif

	if (!fn_range_check("MAIL", nfargs, 0, 2, buff, bufc))
		return;
	if ((nfargs == 0) || !fargs[0] || !fargs[0][0]) {
		count_mail(player, 0, &rc, &uc, &cc);
		safe_ltos(buff, bufc, rc + uc);
		return;
	}
	if (nfargs == 1) {
		if (!is_number(fargs[0])) {
			/* handle the case of wanting to count the number of
			 * messages 
			 */
			if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
				safe_str("#-1 NO SUCH PLAYER", buff, bufc);
				return;
			} else if ((player != playerask) && !Wizard(player)) {
				safe_noperm(buff, bufc);
				return;
			} else {
				count_mail(playerask, 0, &rc, &uc, &cc);
				safe_tprintf_str(buff, bufc, "%d %d %d", rc, uc, cc);
				return;
			}
		} else {
			playerask = player;
			num = atoi(fargs[0]);
		}
	} else {
		if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
			safe_str("#-1 NO SUCH PLAYER", buff, bufc);
			return;
		} else if ((player != playerask) && !God(player)) {
			safe_noperm(buff, bufc);
			return;
		}
		num = atoi(fargs[1]);
	}

	if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER)) {
		safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
		return;
	}
	mp = mail_fetch(playerask, num);
	if (mp != NULL) {
#ifdef RADIX_COMPRESSION
		msgbuff = alloc_lbuf("fun_mail");
		string_decompress(get_mail_message(mp->number), msgbuff);
		safe_str(msgbuff, buff, bufc);
		free_lbuf(msgbuff);
#else
		safe_str(get_mail_message(mp->number), buff, bufc);
#endif
		return;
	}
	/* ran off the end of the list without finding anything */
	safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
}

FUNCTION(fun_mailfrom)
{
	/* This function can take these formats: 1) mailfrom(<num>) 2)
	 * mailfrom(<player>,<num>) It returns the dbref of the player the
	 * mail is from 
	 */
	struct mail *mp;
	dbref playerask;
	int num;

	if (!fn_range_check("MAILFROM", nfargs, 1, 2, buff, bufc))
		return;
	if (nfargs == 1) {
		playerask = player;
		num = atoi(fargs[0]);
	} else {
		if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING) {
			safe_str("#-1 NO SUCH PLAYER", buff, bufc);
			return;
		} else if ((player != playerask) && !Wizard(player)) {
			safe_noperm(buff, bufc);
			return;
		}
		num = atoi(fargs[1]);
	}

	if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER)) {
		safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
		return;
	}
	mp = mail_fetch(playerask, num);
	if (mp != NULL) {
		safe_dbref(buff, bufc, mp->from);
		return;
	}
	/* ran off the end of the list without finding anything */
	safe_str("#-1 NO SUCH MESSAGE", buff, bufc);
}
#endif

/* ---------------------------------------------------------------------------
 * fun_hasattr: does object X have attribute Y.
 */

FUNCTION(fun_hasattr)
{
	dbref thing, aowner;
	int aflags, alen;
	ATTR *attr;
	char *tbuf;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING) {
		safe_nomatch(buff, bufc);
   		return;
	} else if (!Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	attr = atr_str(fargs[1]);
	if (!attr) {
		safe_chr('0', buff, bufc);
		return;
	}
	atr_get_info(thing, attr->number, &aowner, &aflags);
	if (!See_attr(player, thing, attr, aowner, aflags)) {
		safe_chr('0', buff, bufc);
	} else {
		tbuf = atr_get(thing, attr->number, &aowner, &aflags, &alen);
		if (*tbuf) {
			safe_chr('1', buff, bufc);
		} else {
			safe_chr('0', buff, bufc);
		}
		free_lbuf(tbuf);
	}
}

FUNCTION(fun_hasattrp)
{
	dbref thing, aowner;
	int aflags, alen;
	ATTR *attr;
	char *tbuf;

	thing = match_thing(player, fargs[0]);
	if (thing == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!Examinable(player, thing)) {
		safe_noperm(buff, bufc);
		return;
	}
	attr = atr_str(fargs[1]);
	if (!attr) {
		safe_chr('0', buff, bufc);
		return;
	}
	atr_pget_info(thing, attr->number, &aowner, &aflags);
	if (!See_attr(player, thing, attr, aowner, aflags)) {
		safe_chr('0', buff, bufc);
	} else {
		tbuf = atr_pget(thing, attr->number, &aowner, &aflags, &alen);
		if (*tbuf) {
			safe_chr('1', buff, bufc);
		} else {
			safe_chr('0', buff, bufc);
		}
		free_lbuf(tbuf);
	}
}

/* ---------------------------------------------------------------------------
 * fun_default, fun_edefault, and fun_udefault:
 * These check for the presence of an attribute. If it exists, then it
 * is gotten, via the equivalent of get(), get_eval(), or u(), respectively.
 * Otherwise, the default message is used.
 * In the case of udefault(), the remaining arguments to the function
 * are used as arguments to the u().
 */

FUNCTION(fun_default)
{
	dbref thing, aowner;
	int attrib, aflags, alen;
	ATTR *attr;
	char *objname, *atr_gotten, *bp, *str;

	objname = bp = alloc_lbuf("fun_default");
	str = fargs[0];
	exec(objname, &bp, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str,
	     cargs, ncargs);
	*bp = '\0';

	/* First we check to see that the attribute exists on the object.
	 * If so, we grab it and use it. 
	 */

	if (objname != NULL) {
		if (parse_attrib(player, objname, &thing, &attrib) &&
		    (attrib != NOTHING)) {
			attr = atr_num(attrib);
			if (attr && !(attr->flags & AF_IS_LOCK)) {
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
				if (*atr_gotten &&
				check_read_perms(player, thing, attr, aowner,
						 aflags, buff, bufc)) {
					safe_known_str(atr_gotten, alen,
						       buff, bufc);
					free_lbuf(atr_gotten);
					free_lbuf(objname);
					return;
				}
				free_lbuf(atr_gotten);
			}
		}
		free_lbuf(objname);
	}
	/* If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default. 
	 */

	str = fargs[1];
	exec(buff, bufc, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str,
	     cargs, ncargs);
}

FUNCTION(fun_edefault)
{
	dbref thing, aowner;
	int attrib, aflags, alen;
	ATTR *attr;
	char *objname, *atr_gotten, *bp, *str;

	objname = bp = alloc_lbuf("fun_edefault");
	str = fargs[0];
	exec(objname, &bp, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str,
	     cargs, ncargs);
	*bp = '\0';

	/* First we check to see that the attribute exists on the object. 
	 * If so, we grab it and use it. 
	 */

	if (objname != NULL) {
		if (parse_attrib(player, objname, &thing, &attrib) &&
		    (attrib != NOTHING)) {
			attr = atr_num(attrib);
			if (attr && !(attr->flags & AF_IS_LOCK)) {
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);
				if (*atr_gotten &&
				check_read_perms(player, thing, attr, aowner,
						 aflags, buff, bufc)) {
					str = atr_gotten;
					exec(buff, bufc, 0, thing, player, EV_FIGNORE | EV_EVAL,
					     &str, (char **)NULL, 0);
					free_lbuf(atr_gotten);
					free_lbuf(objname);
					return;
				}
				free_lbuf(atr_gotten);
			}
		}
		free_lbuf(objname);
	}
	/* If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default. 
	 */

	str = fargs[1];
	exec(buff, bufc, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str,
	     cargs, ncargs);
}

FUNCTION(fun_udefault)
{
    dbref thing, aowner;
    int aflags, alen, anum, i, j;
    ATTR *ap;
    char *objname, *atext, *str, *bp, *xargs[NUM_ENV_VARS];

    if (nfargs < 2)		/* must have at least two arguments */
	return;

    str = fargs[0];
    objname = bp = alloc_lbuf("fun_udefault");
    exec(objname, &bp, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK,
	 &str, cargs, ncargs);
    *bp = '\0';

    /* First we check to see that the attribute exists on the object.
     * If so, we grab it and use it. 
     */

    if (objname != NULL) {
	if (parse_attrib(player, objname, &thing, &anum)) {
	    if ((anum == NOTHING) || (!Good_obj(thing)))
		ap = NULL;
	    else
		ap = atr_num(anum);
	} else {
	    thing = player;
	    ap = atr_str(objname);
	}
	if (ap) {
	    atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	    if (*atext &&
		check_read_perms(player, thing, ap, aowner, aflags,
				 buff, bufc)) {
		/* Now we have a problem -- we've got to go eval
		 * all of those arguments to the function. 
		 */
		for (i = 2, j = 0; j < NUM_ENV_VARS; i++, j++) {
		    if ((i < nfargs) && fargs[i]) {
			bp = xargs[j] = alloc_lbuf("fun_udefault_args");
			str = fargs[i];
			exec(xargs[j], &bp, 0, player, cause,
			     EV_STRIP | EV_FCHECK | EV_EVAL,
			     &str, cargs, ncargs);
		    } else {
			xargs[j] = NULL;
		    } 
		}
    
		str = atext;
		exec(buff, bufc, 0, thing, cause, EV_FCHECK | EV_EVAL,
		     &str, xargs, nfargs - 2);

		/* Then clean up after ourselves. */

		for (j = 0; j < NUM_ENV_VARS; j++)
		    if (xargs[j])
			free_lbuf(xargs[j]);

		free_lbuf(atext);
		free_lbuf(objname);
		return;
	    }
	    free_lbuf(atext);
	}
	free_lbuf(objname);

    }
    /* If we've hit this point, we've not gotten anything useful, so we 
     * go and evaluate the default. 
     */

    str = fargs[1];
    exec(buff, bufc, 0, player, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str,
	 cargs, ncargs);
}

/* ---------------------------------------------------------------------------
 * fun_findable: can X locate Y
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_findable)
{
	dbref obj = match_thing(player, fargs[0]);
	dbref victim = match_thing(player, fargs[1]);

	if (obj == NOTHING)
		safe_str("#-1 ARG1 NOT FOUND", buff, bufc);
	else if (victim == NOTHING)
		safe_str("#-1 ARG2 NOT FOUND", buff, bufc);
	else
		safe_ltos(buff, bufc, locatable(obj, victim, obj));
}

/* ---------------------------------------------------------------------------
 * fun_visible:  Can X examine Y. If X does not exist, 0 is returned.
 *               If Y, the object, does not exist, 0 is returned. If
 *               Y the object exists, but the optional attribute does
 *               not, X's ability to return Y the object is returned.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_visible)
{
	dbref it, thing, aowner;
	int aflags, atr;
	ATTR *ap;

	if ((it = match_thing(player, fargs[0])) == NOTHING) {
		safe_chr('0', buff, bufc);
		return;
	}
	if (parse_attrib(player, fargs[1], &thing, &atr)) {
		if (atr == NOTHING) {
			safe_ltos(buff, bufc, Examinable(it, thing));
			return;
		}
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		safe_ltos(buff, bufc, See_attr(it, thing, ap, aowner, aflags));
		return;
	}
	thing = match_thing(player, fargs[1]);
	if (!Good_obj(thing)) {
		safe_chr('0', buff, bufc);
		return;
	}
	safe_ltos(buff, bufc, Examinable(it, thing));
}

/* ---------------------------------------------------------------------------
 * fun_elements: given a list of numbers, get corresponding elements from
 * the list.  elements(ack bar eep foof yay,2 4) ==> bar foof
 * The function takes a separator, but the separator only applies to the
 * first list.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_elements)
{
	int nwords, cur;
	char *ptrs[LBUF_SIZE / 2];
	char *wordlist, *s, *r, sep, osep, *oldp;

	svarargs_preamble("ELEMENTS", 4);
	oldp = *bufc;

	/* Turn the first list into an array. */

	wordlist = alloc_lbuf("fun_elements.wordlist");
	strcpy(wordlist, fargs[0]);
	nwords = list2arr(ptrs, LBUF_SIZE / 2, wordlist, sep);

	s = trim_space_sep(fargs[1], ' ');

	/* Go through the second list, grabbing the numbers and finding the
	 * corresponding elements. 
	 */

	do {
		r = split_token(&s, ' ');
		cur = atoi(r) - 1;
		if ((cur >= 0) && (cur < nwords) && ptrs[cur]) {
		    if (oldp != *bufc) {
			print_sep(osep, buff, bufc);
		    }
		    safe_str(ptrs[cur], buff, bufc);
		}
	} while (s);
	free_lbuf(wordlist);
}

/* ---------------------------------------------------------------------------
 * fun_grab: a combination of extract() and match(), sortof. We grab the
 *           single element that we match.
 *
 *   grab(Test:1 Ack:2 Foof:3,*:2)    => Ack:2
 *   grab(Test-1+Ack-2+Foof-3,*o*,+)  => Ack:2
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_grab)
{
	char *r, *s, sep;

	varargs_preamble("GRAB", 3);

	/* Walk the wordstring, until we find the word we want. */

	s = trim_space_sep(fargs[0], sep);
	do {
		r = split_token(&s, sep);
		if (quick_wild(fargs[1], r)) {
			safe_str(r, buff, bufc);
			return;
		}
	} while (s);
}

/* ---------------------------------------------------------------------------
 * fun_scramble:  randomizes the letters in a string.
 */

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_scramble)
{
	int n, i, j;
	char c;

	if (!fargs[0] || !*fargs[0]) {
	    return;
	}

	n = strlen(fargs[0]);
	for (i = 0; i < n; i++) {
	    j = (random() % (n - i)) + i;
	    c = fargs[0][i];
	    fargs[0][i] = fargs[0][j];
	    fargs[0][j] = c;
	}

	safe_str(fargs[0], buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_shuffle: randomize order of words in a list.
 */

/* Borrowed from PennMUSH 1.50 */
static void swap(p, q)
char **p;
char **q;
{
	/* swaps two points to strings */

	char *temp;

	temp = *p;
	*p = *q;
	*q = temp;
}

FUNCTION(fun_shuffle)
{
	char *words[LBUF_SIZE];
	int n, i, j;
	char sep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0]) {
		return;
	}
	svarargs_preamble("SHUFFLE", 3);

	n = list2arr(words, LBUF_SIZE, fargs[0], sep);

	for (i = 0; i < n; i++) {
		j = (random() % (n - i)) + i;
		swap(&words[i], &words[j]);
	}
	arr2list(words, n, buff, bufc, osep);
}

static char ucomp_buff[LBUF_SIZE];
static dbref ucomp_cause;
static dbref ucomp_player;

static int u_comp(s1, s2)
const void *s1, *s2;
{
	/* Note that this function is for use in conjunction with our own 
	 * sane_qsort routine, NOT with the standard library qsort! 
	 */

	char *result, *tbuf, *elems[2], *bp, *str;
	int n;

	if ((mudstate.func_invk_ctr > mudconf.func_invk_lim) ||
	    (mudstate.func_nest_lev > mudconf.func_nest_lim))
		return 0;

	tbuf = alloc_lbuf("u_comp");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	strcpy(tbuf, ucomp_buff);
	result = bp = alloc_lbuf("u_comp");
	str = tbuf;
	exec(result, &bp, 0, ucomp_player, ucomp_cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, &(elems[0]), 2);
	*bp = '\0';
	if (!result)
		n = 0;
	else {
		n = atoi(result);
		free_lbuf(result);
	}
	free_lbuf(tbuf);
	return n;
}

static void sane_qsort(array, left, right, compare)
void *array[];
int left, right;
int (*compare) ();
{
	/* Andrew Molitor's qsort, which doesn't require transitivity between
	 * comparisons (essential for preventing crashes due to
	 * boneheads who write comparison functions where a > b doesn't
	 * mean b < a).  
	 */

	int i, last;
	void *tmp;

      loop:
	if (left >= right)
		return;

	/* Pick something at random at swap it into the leftmost slot */
	/* This is the pivot, we'll put it back in the right spot later */

	i = random() % (1 + (right - left));
	tmp = array[left + i];
	array[left + i] = array[left];
	array[left] = tmp;

	last = left;
	for (i = left + 1; i <= right; i++) {

		/* Walk the array, looking for stuff that's less than our */
		/* pivot. If it is, swap it with the next thing along */

		if ((*compare) (array[i], array[left]) < 0) {
			last++;
			if (last == i)
				continue;

			tmp = array[last];
			array[last] = array[i];
			array[i] = tmp;
		}
	}

	/* Now we put the pivot back, it's now in the right spot, we never */
	/* need to look at it again, trust me.                             */

	tmp = array[last];
	array[last] = array[left];
	array[left] = tmp;

	/* At this point everything underneath the 'last' index is < the */
	/* entry at 'last' and everything above it is not < it.          */

	if ((last - left) < (right - last)) {
		sane_qsort(array, left, last - 1, compare);
		left = last + 1;
		goto loop;
	} else {
		sane_qsort(array, last + 1, right, compare);
		right = last - 1;
		goto loop;
	}
}

FUNCTION(fun_sortby)
{
	char *atext, *list, *ptrs[LBUF_SIZE / 2], sep, osep;
	int nptrs, aflags, alen, anum;
	dbref thing, aowner;
	ATTR *ap;

	if ((nfargs == 0) || !fargs[0] || !*fargs[0]) {
		return;
	}
	svarargs_preamble("SORTBY", 4);

	if (parse_attrib(player, fargs[0], &thing, &anum)) {
		if ((anum == NOTHING) || !Good_obj(thing))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(fargs[0]);
	}

	if (!ap) {
		return;
	}
	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
		free_lbuf(atext);
		return;
	}
	strcpy(ucomp_buff, atext);
	ucomp_player = thing;
	ucomp_cause = cause;

	list = alloc_lbuf("fun_sortby");
	strcpy(list, fargs[1]);
	nptrs = list2arr(ptrs, LBUF_SIZE / 2, list, sep);

	if (nptrs > 1)		/* pointless to sort less than 2 elements */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp);

	arr2list(ptrs, nptrs, buff, bufc, osep);
	free_lbuf(list);
	free_lbuf(atext);
}

/* ---------------------------------------------------------------------------
 * fun_last: Returns last word in a string
 */

FUNCTION(fun_last)
{
	char *s, *last, sep;
	int len, i;

	/* If we are passed an empty arglist return a null string */

	if (nfargs == 0) {
		return;
	}
	varargs_preamble("LAST", 2);
	s = trim_space_sep(fargs[0], sep);	/* trim leading spaces */

	/* If we're dealing with spaces, trim off the trailing stuff */

	if (sep == ' ') {
		len = strlen(s);
		for (i = len - 1; s[i] == ' '; i--) ;
		if (i + 1 <= len)
			s[i + 1] = '\0';
	}
	last = (char *)rindex(s, sep);
	if (last)
		safe_str(++last, buff, bufc);
	else
		safe_str(s, buff, bufc);
}

FUNCTION(fun_matchall)
{
	int wcount;
	char *r, *s, *old, sep, osep, tbuf[8];

	svarargs_preamble("MATCHALL", 4);

	/* SPECIAL CASE: If there's no output delimiter specified, we use
	 * a space, NOT the delimiter given for the list!
	 */
	if (nfargs < 4)
	    osep = ' ';

	old = *bufc;

	/* Check each word individually, returning the word number of all 
	 * that match. If none match, return a null string.
	 */

	wcount = 1;
	s = trim_space_sep(fargs[0], sep);
	do {
		r = split_token(&s, sep);
		if (quick_wild(fargs[1], r)) {
			ltos(tbuf, wcount);
			if (old != *bufc) {
			    print_sep(osep, buff, bufc);
			}
			safe_str(tbuf, buff, bufc);
		}
		wcount++;
	} while (s);
}

/* ---------------------------------------------------------------------------
 * fun_ports: Returns a list of ports for a user.
 */

/* Borrowed from TinyMUSH 2.2 */
FUNCTION(fun_ports)
{
	dbref target;

	if (!Wizard(player)) {
		return;
	}
	target = lookup_player(player, fargs[0], 1);
	if (!Good_obj(target) || !Connected(target)) {
		return;
	}
	make_portlist(player, target, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * fun_mix: Like map, but operates on two lists or more lists simultaneously,
 * passing the elements as %0, %1, %2, etc.
 */

FUNCTION(fun_mix)
{
    dbref aowner, thing;
    int aflags, alen, anum, i, lastn, nwords, wc;
    ATTR *ap;
    char *str, *atext, *os[10], *atextbuf, *bb_p, sep;
    char *cp[10];
    int count[LBUF_SIZE / 2];
    char tmpbuf[2];

    /* Check to see if we have an appropriate number of arguments.
     * If there are more than three arguments, the last argument is
     * ALWAYS assumed to be a delimiter.
     */

    if (!fn_range_check("MIX", nfargs, 3, 12, buff, bufc)) {
	return;
    }
    if (nfargs < 4) {
	sep = ' ';
	lastn = nfargs - 1;
    } else if (!delim_check(fargs, nfargs, nfargs, &sep, buff, bufc, 0,
			    player, cause, cargs, ncargs, 0)) {
	return;
    } else {
	lastn = nfargs - 2;
    }

    /* Get the attribute, check the permissions. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || !Good_obj(thing))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    if (!ap) {
	return;
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
    if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }

    for (i = 0; i < 10; i++)
	cp[i] = NULL;

    bb_p = *bufc;

    /* process the lists, one element at a time. */

    for (i = 1; i <= lastn; i++) {
	cp[i] = trim_space_sep(fargs[i], sep);
    }
    nwords = count[1] = countwords(cp[1], sep);
    for (i = 2; i<= lastn; i++) {
	count[i] = countwords(cp[i], sep);
	if (count[i] > nwords)
	    nwords = count[i];
    }
    atextbuf = alloc_lbuf("fun_mix");

    for (wc = 0;
	 (wc < nwords) && (mudstate.func_invk_ctr < mudconf.func_invk_lim);
	 wc++) {
	for (i = 1; i <= lastn; i++) {
	    if (count[i]) {
		os[i - 1] = split_token(&cp[i], sep);
	    } else {
		tmpbuf[0] = '\0';
		os[i - 1] = tmpbuf;
	    }
	}
	strcpy(atextbuf, atext);

	if (*bufc != bb_p)
	    safe_chr(sep, buff, bufc);

	str = atextbuf;
	
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		      &str, &(os[0]), lastn);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/* ---------------------------------------------------------------------------
 * fun_step: A little like a fusion of iter() and mix(), it takes elements
 * of a list X at a time and passes them into a single function as %0, %1,
 * etc.   step(<attribute>,<list>,<step size>,<delim>,<outdelim>)
 */

FUNCTION(fun_step)
{
    ATTR *ap;
    dbref aowner, thing;
    int aflags, alen, anum;
    char *atext, *str, *cp, *atextbuf, *bb_p, *os[10];
    char sep, osep;
    int step_size, i;

    svarargs_preamble("STEP", 5);

    step_size = atoi(fargs[2]);
    if ((step_size < 1) || (step_size > NUM_ENV_VARS)) {
	notify(player, "Illegal step size.");
	return;
    }

    /* Get attribute. Check permissions. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || !Good_obj(thing))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }
    if (!ap)
	return;

    atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
    if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }

    cp = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_step");
    bb_p = *bufc;
    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {
	if (*bufc != bb_p) {
	    print_sep(osep, buff, bufc);
	}
	for (i = 0; cp && (i < step_size); i++)
	    os[i] = split_token(&cp, sep);
	strcpy(atextbuf, atext);
	str = atextbuf;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	     &str, &(os[0]), i);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}


/* ---------------------------------------------------------------------------
 * fun_foreach: like map(), but it operates on a string, rather than on a list,
 * calling a user-defined function for each character in the string.
 * No delimiter is inserted between the results.
 */

FUNCTION(fun_foreach)
{
    dbref aowner, thing;
    int aflags, alen, anum;
    ATTR *ap;
    char *str, *atext, *atextbuf, *cp, *cbuf;
    char start_token, end_token;
    int in_string = 1;

    if (!fn_range_check("FOREACH", nfargs, 2, 4, buff, bufc))
	return;

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || !Good_obj(thing))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    if (!ap) {
	return;
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
    if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }
    atextbuf = alloc_lbuf("fun_foreach");
    cbuf = alloc_lbuf("fun_foreach.cbuf");
    cp = trim_space_sep(fargs[1], ' ');

    start_token = '\0';
    end_token = '\0';

    if (nfargs > 2) {
	in_string = 0;
	start_token = *fargs[2];
    }
    if (nfargs > 3) {
	end_token = *fargs[3];
    }

    while (cp && *cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim)) {

	if (!in_string) {
	    /* Look for a start token. */
	    while (*cp && (*cp != start_token)) {
		safe_chr(*cp, buff, bufc);
		cp++;
	    }
	    if (!*cp)
		break;
	    /* Skip to the next character. Don't copy the start token. */
	    cp++;
	    if (!*cp)
		break;
	    in_string = 1;
	}
	if (*cp == end_token) {
	    /* We've found an end token. Skip over it. Note that it's
	     * possible to have a start and end token next to one
	     * another.
	     */
	    cp++;
	    in_string = 0;
	    continue;
	}

	cbuf[0] = *cp++;
	cbuf[1] = '\0';
	strcpy(atextbuf, atext);
	str = atextbuf; 
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
		      &str, &cbuf, 1);
    }

    free_lbuf(atextbuf);
    free_lbuf(atext);
    free_lbuf(cbuf);
}

/* ---------------------------------------------------------------------------
 * fun_munge: combines two lists in an arbitrary manner.
 */

/* Borrowed from TinyMUSH 2.2 */
FUNCTION(fun_munge)
{
	dbref aowner, thing;
	int aflags, alen, anum, nptrs1, nptrs2, nresults, i, j;
	ATTR *ap;
	char *list1, *list2, *rlist;
	char *ptrs1[LBUF_SIZE / 2], *ptrs2[LBUF_SIZE / 2], *results[LBUF_SIZE / 2];
	char *atext, *bp, *str, sep, osep, *oldp;

	oldp = *bufc;
	if ((nfargs == 0) || !fargs[0] || !*fargs[0]) {
		return;
	}
	svarargs_preamble("MUNGE", 5);

	/* Find our object and attribute */

	if (parse_attrib(player, fargs[0], &thing, &anum)) {
		if ((anum == NOTHING) || !Good_obj(thing))
			ap = NULL;
		else
			ap = atr_num(anum);
	} else {
		thing = player;
		ap = atr_str(fargs[0]);
	}

	if (!ap) {
		return;
	}
	atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
	if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
		free_lbuf(atext);
		return;
	}
	/* Copy our lists and chop them up. */

	list1 = alloc_lbuf("fun_munge.list1");
	list2 = alloc_lbuf("fun_munge.list2");
	strcpy(list1, fargs[1]);
	strcpy(list2, fargs[2]);
	nptrs1 = list2arr(ptrs1, LBUF_SIZE / 2, list1, sep);
	nptrs2 = list2arr(ptrs2, LBUF_SIZE / 2, list2, sep);

	if (nptrs1 != nptrs2) {
		safe_str("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
		free_lbuf(atext);
		free_lbuf(list1);
		free_lbuf(list2);
		return;
	}
	/* Call the u-function with the first list as %0. */

	bp = rlist = alloc_lbuf("fun_munge");
	str = atext;
	exec(rlist, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	     &fargs[1], 1);
	*bp = '\0';

	/* Now that we have our result, put it back into array form. Search
	 * through list1 until we find the element position, then 
	 * copy the corresponding element from list2. 
	 */

	nresults = list2arr(results, LBUF_SIZE / 2, rlist, sep);

	for (i = 0; i < nresults; i++) {
		for (j = 0; j < nptrs1; j++) {
			if (!strcmp(results[i], ptrs1[j])) {
			        if (*bufc != oldp) {
				    print_sep(osep, buff, bufc);
				}
				safe_str(ptrs2[j], buff, bufc);
				ptrs1[j][0] = '\0';
				break;
			}
		}
	}
	free_lbuf(atext);
	free_lbuf(list1);
	free_lbuf(list2);
	free_lbuf(rlist);
}

FUNCTION(fun_die)
{
	int n, die, count;
	int total = 0;

	if (!fargs[0] || !fargs[1]) {
	    safe_chr('0', buff, bufc);
	    return;
	}

	n = atoi(fargs[0]);
	die = atoi(fargs[1]);

	if ((n == 0) || (die <= 0)) {
	    safe_chr('0', buff, bufc);
	    return;
	}

	if ((n < 1) || (n > 100)) {
		safe_str("#-1 NUMBER OUT OF RANGE", buff, bufc);
		return;
	}
	for (count = 0; count < n; count++)
		total += (int) (makerandom() * die) + 1;

	safe_ltos(buff, bufc, total);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_lit)
{
	/* Just returns the argument, literally */
	safe_str(fargs[0], buff, bufc);
}

/* shl() and shr() borrowed from PennMUSH 1.50 */
FUNCTION(fun_shl)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) << atoi(fargs[1]));
}

FUNCTION(fun_shr)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) >> atoi(fargs[1]));
}

FUNCTION(fun_band)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) & atoi(fargs[1]));
}

FUNCTION(fun_bor)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) | atoi(fargs[1]));
}

FUNCTION(fun_bnand)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) & ~(atoi(fargs[1])));
}

/* ------------------------------------------------------------------------
 */

FUNCTION(fun_strcat)
{
	int i;
	
	safe_str(fargs[0], buff, bufc);
	for (i = 1; i < nfargs; i++) {
		safe_str(fargs[i], buff, bufc);
	}
}

/* grep() and grepi() code borrowed from PennMUSH 1.50 */
char *grep_util(player, thing, pattern, lookfor, len, insensitive)
dbref player, thing;
char *pattern;
char *lookfor;
int len;
int insensitive;
{
	/* returns a list of attributes which match <pattern> on <thing> 
	 * whose contents have <lookfor> 
	 */
	dbref aowner;
	char *tbuf1, *buf, *text, *attrib;
	char *bp, *bufc;
	int found;
	int ca, aflags, alen;

	bp = tbuf1 = alloc_lbuf("grep_util");
	bufc = buf = alloc_lbuf("grep_util.parse_attrib");
	safe_tprintf_str(buf, &bufc, "#%d/%s", thing, pattern);
	olist_push();
	if (parse_attrib_wild(player, buf, &thing, 0, 0, 1)) {
		for (ca = olist_first(); ca != NOTHING; ca = olist_next()) {
			attrib = atr_get(thing, ca, &aowner, &aflags, &alen);
			text = attrib;
			found = 0;
			while (*text && !found) {
				if ((!insensitive && !strncmp(lookfor, text, len)) ||
				    (insensitive && !strncasecmp(lookfor, text, len)))
					found = 1;
				else
					text++;
			}

			if (found) {
				if (bp != tbuf1)
					safe_chr(' ', tbuf1, &bp);

				safe_str((char *)(atr_num(ca))->name, tbuf1, &bp);
			}
			free_lbuf(attrib);
		}
	}
	free_lbuf(buf);
	*bp = '\0';
	olist_pop();
	return tbuf1;
}

FUNCTION(fun_grep)
{
	char *tp;

	dbref it = match_thing(player, fargs[0]);

	if (it == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!(Examinable(player, it))) {
		safe_noperm(buff, bufc);
		return;
	}
	/* make sure there's an attribute and a pattern */
	if (!fargs[1] || !*fargs[1]) {
		safe_str("#-1 NO SUCH ATTRIBUTE", buff, bufc);
		return;
	}
	if (!fargs[2] || !*fargs[2]) {
		safe_str("#-1 INVALID GREP PATTERN", buff, bufc);
		return;
	}
	tp = grep_util(player, it, fargs[1], fargs[2], strlen(fargs[2]), 0);
	safe_str(tp, buff, bufc);
	free_lbuf(tp);
}

FUNCTION(fun_grepi)
{
	char *tp;

	dbref it = match_thing(player, fargs[0]);

	if (it == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!(Examinable(player, it))) {
		safe_noperm(buff, bufc);
		return;
	}
	/* make sure there's an attribute and a pattern */
	if (!fargs[1] || !*fargs[1]) {
		safe_str("#-1 NO SUCH ATTRIBUTE", buff, bufc);
		return;
	}
	if (!fargs[2] || !*fargs[2]) {
		safe_str("#-1 INVALID GREP PATTERN", buff, bufc);
		return;
	}
	tp = grep_util(player, it, fargs[1], fargs[2], strlen(fargs[2]), 1);
	safe_str(tp, buff, bufc);
	free_lbuf(tp);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_art)
{
/* checks a word and returns the appropriate article, "a" or "an" */
	char c = tolower(*fargs[0]);

	if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
		safe_known_str("an", 2, buff, bufc);
	else
		safe_chr('a', buff, bufc);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_alphamax)
{
	char *amax;
	int i = 1;

	if (!fargs[0]) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	} else
		amax = fargs[0];

	while ((i < 10) && fargs[i]) {
		amax = (strcmp(amax, fargs[i]) > 0) ? amax : fargs[i];
		i++;
	}

	safe_str(amax, buff, bufc);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_alphamin)
{
	char *amin;
	int i = 1;

	if (!fargs[0]) {
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	} else
		amin = fargs[0];

	while ((i < 10) && fargs[i]) {
		amin = (strcmp(amin, fargs[i]) < 0) ? amin : fargs[i];
		i++;
	}

	safe_str(amin, buff, bufc);
}

/* Borrowed from PennMUSH 1.50 */

FUNCTION(fun_valid)
{
/* Checks to see if a given <something> is valid as a parameter of a
 * given type (such as an object name).
 */

	if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1]) {
		safe_chr('0', buff, bufc);
	} else if (!strcasecmp(fargs[0], "name"))
		safe_ltos(buff, bufc, ok_name(fargs[1]));
	else
		safe_nothing(buff, bufc);
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_hastype)
{
	dbref it = match_thing(player, fargs[0]);

	if (it == NOTHING) {
		safe_nomatch(buff, bufc);
		return;
	}
	if (!fargs[1] || !*fargs[1]) {
		safe_str("#-1 NO SUCH TYPE", buff, bufc);
		return;
	}
	switch (*fargs[1]) {
	case 'r':
	case 'R':
		safe_chr((Typeof(it) == TYPE_ROOM) ? '1' : '0', buff, bufc);
		break;
	case 'e':
	case 'E':
		safe_chr((Typeof(it) == TYPE_EXIT) ? '1' : '0', buff, bufc);
		break;
	case 'p':
	case 'P':
		safe_chr((Typeof(it) == TYPE_PLAYER) ? '1' : '0', buff, bufc);
		break;
	case 't':
	case 'T':
		safe_chr((Typeof(it) == TYPE_THING) ? '1' : '0', buff, bufc);
		break;
	default:
		safe_str("#-1 NO SUCH TYPE", buff, bufc);
		break;
	};
}

/* Borrowed from PennMUSH 1.50 */
FUNCTION(fun_lparent)
{
	dbref it;
	dbref par;
	char tbuf1[20];
	int i;

	it = match_thing(player, fargs[0]);
	if (!Good_obj(it)) {
		safe_nomatch(buff, bufc);
		return;
	} else if (!(Examinable(player, it))) {
		safe_noperm(buff, bufc);
		return;
	}
	sprintf(tbuf1, "#%d", it);
	safe_str(tbuf1, buff, bufc);
	par = Parent(it);

	i = 1;
	while (Good_obj(par) && Examinable(player, it) &&
	    (i < mudconf.parent_nest_lim)) {
	    sprintf(tbuf1, " #%d", par);
	    safe_str(tbuf1, buff, bufc);
	    it = par;
	    par = Parent(par);
	    i++;
	}
}

/* ---------------------------------------------------------------------------
 * Object stack functions.
 */

void stack_clr(thing)
    dbref thing;
{
    STACK *sp, *tp, *xp;

    sp = stack_get(thing);
    if (sp) {
	for (tp = sp; tp != NULL; ) {
	    XFREE(tp->data, "stack_clr_data");
	    xp = tp;
	    tp = tp->next;
	    XFREE(xp, "stack_clr");
	}
	nhashdelete(thing, &mudstate.objstack_htab);
	s_StackCount(thing, 0);
    }
}

static void stack_set(thing, sp)
    dbref thing;
    STACK *sp;
{
    STACK *xsp;
    int stat;

    if (!sp) {
	nhashdelete(thing, &mudstate.objstack_htab);
	s_StackCount(thing, StackCount(thing) - 1);
	return;
    }

    if (StackCount(thing) + 1 > mudconf.stack_lim) {
	XFREE(sp->data, "stack_max_data");
	XFREE(sp, "stack_max");
	return;
    }

    xsp = stack_get(thing);
    if (xsp) {
	stat = nhashrepl(thing, (int *) sp, &mudstate.objstack_htab);
    } else {
	stat = nhashadd(thing, (int *) sp, &mudstate.objstack_htab);
	s_StackCount(thing, StackCount(thing) + 1);
	Set_Max(mudstate.max_stacks, mudstate.objstack_htab.entries);
    }
    if (stat < 0) {		/* failure for some reason */
	STARTLOG(LOG_BUGS, "STK", "SET")
	    log_name(thing);
	ENDLOG
	stack_clr(thing);
    }
}

FUNCTION(fun_empty)
{
    dbref it;

    xvarargs_preamble("EMPTY", 0, 1);

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }

    stack_clr(it);
}

FUNCTION(fun_items)
{
    dbref it;
    int i;
    STACK *sp;

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }

    for (i = 0, sp = stack_get(it); sp != NULL; sp = sp->next, i++)
	;
    safe_ltos(buff, bufc, i);
}
    

FUNCTION(fun_push)
{
    dbref it;
    char *data;
    STACK *sp;

    xvarargs_preamble("PUSH", 1, 2);

    if (!fargs[1]) {
	it = player;
	data = fargs[0];
    } else {
	stack_object(player, it);
	data = fargs[1];
    }

    sp = (STACK *) XMALLOC(sizeof(STACK), "stack_push");
    if (!sp)			/* out of memory, ouch */
	return;
    sp->next = stack_get(it);
    sp->data = (char *) XMALLOC(sizeof(char) * (strlen(data) + 1),
				"stack_push_data");
    if (! sp->data)
	return;
    strcpy(sp->data, data);
    stack_set(it, sp);
}

FUNCTION(fun_dup)
{
    dbref it;
    STACK *hp;			/* head of stack */
    STACK *tp;			/* temporary stack pointer */
    STACK *sp;			/* new stack element */
    int pos, count = 0;

    xvarargs_preamble("DUP", 0, 2);

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }

    if (!fargs[1] || !*fargs[1]) {
	pos = 0;
    } else {
	pos = atoi(fargs[1]);
    }

    hp = stack_get(it);
    for (tp = hp; (count != pos) && (tp != NULL); count++, tp = tp->next)
	;
    if (!tp) {
	notify_quiet(player, "No such item on stack.");
	return;
    }

    sp = (STACK *) XMALLOC(sizeof(STACK), "stack_dup");
    if (!sp)
	return;
    sp->next = hp;
    sp->data = (char *) XMALLOC(sizeof(char) * (strlen(tp->data) + 1),
				"stack_dup_data");
    if (!sp->data)
	return;
    strcpy(sp->data, tp->data);
    stack_set(it, sp);
}

FUNCTION(fun_swap)
{
    dbref it;
    STACK *sp, *tp;

    xvarargs_preamble("SWAP", 0, 1);

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }

    sp = stack_get(it);
    if (!sp || (sp->next == NULL)) {
	notify_quiet(player, "Not enough items on stack.");
	return;
    }

    tp = sp->next;
    sp->next = tp->next;
    tp->next = sp;
    stack_set(it, tp);
}

static void handle_pop(player, cause, buff, bufc, fargs, flag)
    dbref player, cause;
    char *buff;
    char **bufc;
    char *fargs[];
    int flag;			/* if flag, don't copy */
{
    dbref it;
    int pos, count = 0;
    STACK *sp;
    STACK *prev = NULL;

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }
    if (!fargs[1] || !*fargs[1]) {
	pos = 0;
    } else {
	pos = atoi(fargs[1]);
    }

    sp = stack_get(it);
    if (!sp)
	return;

    while (count != pos) {
	if (!sp)
	    return;
	prev = sp;
	sp = sp->next;
	count++;
    }
    if (!sp)
	return;

    if (!flag) {
	safe_str(sp->data, buff, bufc);
    }
    if (count == 0) {
	stack_set(it, sp->next);
    } else {
	prev->next = sp->next;
    }
    XFREE(sp->data, "stack_pop_data");
    XFREE(sp, "stack_pop");
    s_StackCount(it, StackCount(it) - 1);
}

FUNCTION(fun_pop)
{
    xvarargs_preamble("POP", 0, 2);
    handle_pop(player, cause, buff, bufc, fargs, 0);
}

FUNCTION(fun_toss)
{
    xvarargs_preamble("TOSS", 0, 2);
    handle_pop(player, cause, buff, bufc, fargs, 1);
}

FUNCTION(fun_popn)
{
    dbref it;
    int pos, nitems, i, count = 0, over = 0, first = 0;
    STACK *sp, *tp, *xp;
    STACK *prev = NULL;
    char sep;

    varargs_preamble("POPN", 4);

    stack_object(player, it);
    pos = atoi(fargs[1]);
    nitems = atoi(fargs[2]);

    sp = stack_get(it);
    if (!sp)
	return;

    while (count != pos) {
	if (!sp)
	    return;
	prev = sp;
	sp = sp->next;
	count++;
    }
    if (!sp)
	return;

    /* We've now hit the start item, the first item. Copy 'em off. */

    for (i = 0, tp = sp; (i < nitems) && (tp != NULL); i++) {
	if (!over) {
	    /* We have to pop off the items regardless of whether
	     * or not there's an overflow, but we can save ourselves
	     * some copying if so.
	     */
	    if (!first) {
		safe_chr(sep, buff, bufc);
		first = 1;
	    }
	    over = safe_str(tp->data, buff, bufc);
	}
	xp = tp;
	tp = tp->next;
	XFREE(xp->data, "stack_popn_data");
	XFREE(xp, "stack_popn");
	s_StackCount(it, StackCount(it) - 1);
    }

    /* Relink the chain. */

    if (count == 0) {
	stack_set(it, tp);
    } else {
	prev->next = tp;
    }
}

FUNCTION(fun_peek)
{
    dbref it;
    int pos, count = 0;
    STACK *sp;

    xvarargs_preamble("POP", 0, 2);

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }
    if (!fargs[1] || !*fargs[1]) {
	pos = 0;
    } else {
	pos = atoi(fargs[1]);
    }

    sp = stack_get(it);
    if (!sp)
	return;

    while (count != pos) {
	if (!sp)
	    return;
	sp = sp->next;
	count++;
    }
    if (!sp)
	return;

    safe_str(sp->data, buff, bufc);
}

FUNCTION(fun_lstack)
{
    char sep;
    dbref it;
    STACK *sp;
    char *bp;
    int over = 0, first = 1;

    mvarargs_preamble("LSTACK", 0, 2);

    if (!fargs[0]) {
	it = player;
    } else {
	stack_object(player, it);
    }

    bp = buff;
    for (sp = stack_get(it); (sp != NULL) && !over; sp = sp->next) {
	if (!first) {
	    safe_chr(sep, buff, bufc);
	} else {
	    first = 0;
	}
	over = safe_str(sp->data, buff, bufc);
    }
}

/* ---------------------------------------------------------------------------
 * fun_x: Returns a variable. x(<variable name>)
 * fun_setx: Sets a variable. xset(<variable name>,<value>)
 * fun_xvars: Takes a list, parses it, sets it into variables.
 *            xvars(<space-separated variable list>,<list>,<delimiter>)
 * fun_let: Takes a list of variables and their values, sets them, executes
 *          a function, and clears out the variables. (Scheme/ML-like.)
 *          If <list> is empty, the values are reset to null.
 *          let(<space-separated var list>,<list>,<body>,<delimiter>)
 * fun_lvars: Shows a list of variables associated with that object.
 * fun_clearvars: Clears all variables associated with that object.
 */


static void set_xvar(obj, name, data)
    dbref obj;
    char *name;
    char *data;
{
    VARENT *xvar;
    char tbuf[SBUF_SIZE], *tp, *p;

    /* If we don't have at least one character in the name, toss it. */

    if (!name || !*name)
	return;

    /* Variable string is '<dbref number minus #>.<variable name>'. We
     * lowercase all names. Note that we're going to end up automatically
     * truncating long names.
     */

    tp = tbuf;
    safe_ltos(tbuf, &tp, obj);
    safe_sb_chr('.', tbuf, &tp);
    for (p = name; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(name, tbuf, &tp);
    *tp = '\0';

    /* Search for it. If it exists, replace it. If we get a blank string,
     * delete the variable.
     */

    if ((xvar = (VARENT *) hashfind(tbuf, &mudstate.vars_htab))) {
	if (xvar->text) {
	    XFREE(xvar->text, "xvar_data");
	}
	if (data && *data) {
	    xvar->text = (char *) XMALLOC(sizeof(char) * (strlen(data) + 1),
					  "xvar_data");
	    if (!xvar->text)
		return;		/* out of memory */
	    strcpy(xvar->text, data);
	} else {
	    xvar->text = NULL;
	    XFREE(xvar, "xvar_struct");
	    hashdelete(tbuf, &mudstate.vars_htab);
	    s_VarsCount(obj, VarsCount(obj) - 1);
	}
    } else {

	/* We haven't found it. If it's non-empty, set it, provided we're
	 * not running into a limit on the number of vars per object.
	 */

	if (VarsCount(obj) + 1 > mudconf.numvars_lim)
	    return;

	if (data && *data) {
	    xvar = (VARENT *) XMALLOC(sizeof(VARENT), "xvar_struct");
	    if (!xvar)
		return;		/* out of memory */
	    xvar->text = (char *) XMALLOC(sizeof(char) * (strlen(data) + 1),
					  "xvar_data");
	    if (!xvar->text)
		return;		/* out of memory */
	    strcpy(xvar->text, data);
	    hashadd(tbuf, (int *) xvar, &mudstate.vars_htab);
	    s_VarsCount(obj, VarsCount(obj) + 1);
	    Set_Max(mudstate.max_vars, mudstate.vars_htab.entries);
	}
    }
}


static void clear_xvars(obj, xvar_names, n_xvars)
    dbref obj;
    char **xvar_names;
    int n_xvars;
{
    /* Clear out an array of variable names. */

    char pre[SBUF_SIZE], tbuf[SBUF_SIZE], *tp, *p;
    VARENT *xvar;
    int i;

    /* Build our dbref bit first. */

    tp = pre;
    safe_ltos(pre, &tp, obj);
    safe_sb_chr('.', pre, &tp);
    *tp = '\0';

    /* Go clear stuff. */

    for (i = 0; i < n_xvars; i++) {

	for (p = xvar_names[i]; *p; p++)
	    *p = ToLower(*p);
	tp = tbuf;
	safe_sb_str(pre, tbuf, &tp);
	safe_sb_str(xvar_names[i], tbuf, &tp);
	*tp = '\0';

	if ((xvar = (VARENT *) hashfind(tbuf, &mudstate.vars_htab))) {
	    if (xvar->text) {
		XFREE(xvar->text, "xvar_data");
		xvar->text = NULL;
	    }
	    XFREE(xvar, "xvar_struct");
	    hashdelete(tbuf, &mudstate.vars_htab);
	}
    }

    s_VarsCount(obj, VarsCount(obj) - n_xvars);
}


void xvars_clr(player)
    dbref player;
{
    char tbuf[SBUF_SIZE], *tp;
    HASHTAB *htab;
    HASHENT *hptr, *last, *next;
    int i, len;
    VARENT *xvar;

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);

    htab = &mudstate.vars_htab;
    for (i = 0; i < htab->hashsize; i++) {
	last = NULL;
	for (hptr = htab->entry->element[i]; hptr != NULL; last = hptr) {
	    next = hptr->next;
	    if (!strncmp(tbuf, hptr->target, len)) {
		if (last == NULL)
		    htab->entry->element[i] = next;
		else
		    last->next = next;
		xvar = (VARENT *) hptr->data;
		XFREE(xvar->text, "xvar_data");
		XFREE(xvar, "xvar_struct");
		free(hptr->target);
		free(hptr);
		htab->deletes++;
		htab->entries--;
		if (htab->entry->element[i] == NULL)
		    htab->nulls++;
	    }
	    hptr = next;
	}
    }

    s_VarsCount(player, 0);
}


FUNCTION(fun_x)
{
    VARENT *xvar;
    char tbuf[SBUF_SIZE], *tp, *p;

    /* Variable string is '<dbref number minus #>.<variable name>' */

    tp = tbuf;
    safe_ltos(tbuf, &tp, player);
    safe_sb_chr('.', tbuf, &tp);
    for (p = fargs[0]; *p; p++)
	*p = ToLower(*p);
    safe_sb_str(fargs[0], tbuf, &tp);
    *tp = '\0';

    if ((xvar = (VARENT *) hashfind(tbuf, &mudstate.vars_htab)))
	safe_str(xvar->text, buff, bufc);
}


FUNCTION(fun_setx)
{
    set_xvar(player, fargs[0], fargs[1]);
}


FUNCTION(fun_xvars)
{
    char *xvar_names[LBUF_SIZE / 2], *elems[LBUF_SIZE / 2];
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    int i;
    char sep;

    varargs_preamble("XVARS", 3);
   
    varlist = alloc_lbuf("fun_xvars.vars");
    strcpy(varlist, fargs[0]);
    n_xvars = list2arr(xvar_names, LBUF_SIZE / 2, varlist, ' ');

    if (n_xvars == 0) {
	free_lbuf(varlist);
	return;
    }

    if (!fargs[1] || !*fargs[1]) {
	/* Empty list, clear out the data. */
	clear_xvars(player, xvar_names, n_xvars);
	free_lbuf(varlist);
	return;
    }

    elemlist = alloc_lbuf("fun_xvars.elems");
    strcpy(elemlist, fargs[1]);
    n_elems = list2arr(elems, LBUF_SIZE / 2, elemlist, sep);

    if (n_elems != n_xvars) {
	safe_str("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
	free_lbuf(varlist);
	free_lbuf(elemlist);
	return;
    }

    for (i = 0; i < n_elems; i++) {
	set_xvar(player, xvar_names[i], elems[i]);
    }

    free_lbuf(varlist);
    free_lbuf(elemlist);
}


FUNCTION(fun_let)
{
    char *xvar_names[LBUF_SIZE / 2], *elems[LBUF_SIZE / 2];
    char *old_xvars[LBUF_SIZE / 2];
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    char *str, *bp, *p;
    char pre[SBUF_SIZE], tbuf[SBUF_SIZE], *tp;
    VARENT *xvar;
    int i;
    char sep;

    varargs_preamble("LET", 4);

    if (!fargs[0] || !*fargs[0])
	return;
   
    varlist = bp = alloc_lbuf("fun_let.vars");
    str = fargs[0];
    exec(varlist, &bp, 0, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str,
	 cargs, ncargs);
    *bp = '\0';
    n_xvars = list2arr(xvar_names, LBUF_SIZE / 2, varlist, ' ');

    if (n_xvars == 0) {
	free_lbuf(varlist);
	return;
    }

    /* Lowercase our variable names. */

    for (i = 0, p = xvar_names[i]; *p; p++)
	*p = ToLower(*p);

    /* Save our original values. Copying this stuff into an array is
     * unnecessarily expensive because we allocate and free memory 
     * that we could theoretically just trade pointers around for --
     * but this way is cleaner.
     */

    tp = pre;
    safe_ltos(pre, &tp, player);
    safe_sb_chr('.', pre, &tp);
    *tp = '\0';

    for (i = 0; i < n_xvars; i++) {

	tp = tbuf;
	safe_sb_str(pre, tbuf, &tp);
	safe_sb_str(xvar_names[i], tbuf, &tp);
	*tp = '\0';

	if ((xvar = (VARENT *) hashfind(tbuf, &mudstate.vars_htab))) {
	    if (xvar->text)
		old_xvars[i] = (char *) strdup(xvar->text);
	    else
		old_xvars[i] = NULL;
	} else {
	    old_xvars[i] = NULL;
	}
    }

    if (fargs[1] && *fargs[1]) {

	/* We have data, so we should initialize variables to their values,
	 * ala xvars(). However, unlike xvars(), if we don't get a list,
	 * we just leave the values alone (we don't clear them out).
	 */

	elemlist = bp = alloc_lbuf("fun_let.elems");
	str = fargs[1];
	exec(elemlist, &bp, 0, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL,
	     &str, cargs, ncargs);
	*bp = '\0';
	n_elems = list2arr(elems, LBUF_SIZE / 2, elemlist, sep);

	if (n_elems != n_xvars) {
	    safe_str("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
	    free_lbuf(varlist);
	    free_lbuf(elemlist);
	    return;
	}

	for (i = 0; i < n_elems; i++) {
	    set_xvar(player, xvar_names[i], elems[i]);
	}

	free_lbuf(elemlist);

    }

    /* Now we go to execute our function body. */

    str = fargs[2];
    exec(buff, bufc, 0, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str,
	 cargs, ncargs);

    /* Restore the old values. */

    for (i = 0; i < n_xvars; i++) {
	set_xvar(player, xvar_names[i], old_xvars[i]);
	if (old_xvars[i])
	    free(old_xvars[i]);
    }

    free_lbuf(varlist);
}


FUNCTION(fun_lvars)
{
    print_htab_matches(player, &mudstate.vars_htab, buff, bufc);
}


FUNCTION(fun_clearvars)
{
    /* This is computationally expensive. Necessary, but its use should
     * be avoided if possible.
     */

    xvars_clr(player);
}

/* ---------------------------------------------------------------------------
 * fun_regparse: Slurp a string into up to ten named variables ($0 - $9).
 *               Unlike regmatch(), this returns no value.
 *               regparse(string, pattern, named vars)
 */

FUNCTION(fun_regparse)
{
    int i, nqregs, len;
    char *qregs[NSUBEXP];
    char matchbuf[LBUF_SIZE];
    regexp *re;
    int matched;

    if ((re = regcomp(fargs[1])) == NULL) {
	/* Matching error. */
	notify_quiet(player, (const char *) regexp_errbuf);
	return;
    }

    matched = (int) regexec(re, fargs[0]);

    nqregs = list2arr(qregs, NSUBEXP, fargs[2], ' ');
    for (i = 0; i < nqregs; i++) {
	if (qregs[i] && *qregs[i]) {
	    if (!matched || !re->startp[i] || !re->endp[i]) {
		set_xvar(player, qregs[i], NULL);
	    } else {
		len = re->endp[i] - re->startp[i];
		if (len > LBUF_SIZE - 1)
		    len = LBUF_SIZE - 1;
		else if (len < 0)
		    len = 0;
		strncpy(matchbuf, re->startp[i], len);
		matchbuf[len] = '\0';
		set_xvar(player, qregs[i], matchbuf);
	    }
	}
    }

    free(re);
}

/* ---------------------------------------------------------------------------
 * fun_regmatch: Return 0 or 1 depending on whether or not a regular
 * expression matches a string. If a third argument is specified, dump
 * the results of a regexp pattern match into a set of arbitrary r()-registers.
 *
 * regmatch(string, pattern, list of registers)
 * If the number of matches exceeds the registers, those bits are tossed
 * out.
 * If -1 is specified as a register number, the matching bit is tossed.
 * Therefore, if the list is "-1 0 3 5", the regexp $0 is tossed, and
 * the regexp $1, $2, and $3 become r(0), r(3), and r(5), respectively.
 *
 */

FUNCTION(fun_regmatch)
{
int i, nqregs, curq, len;
char *qregs[NSUBEXP];
regexp *re;
int matched;

	if (!fn_range_check("REGMATCH", nfargs, 2, 3, buff, bufc))
		return;

	if ((re = regcomp(fargs[1])) == NULL) {
		/* Matching error. */
		notify_quiet(player, (const char *) regexp_errbuf);
		safe_chr('0', buff, bufc);
		return;
	}

	matched = (int) regexec(re, fargs[0]);
	safe_ltos(buff, bufc, matched);

	/* If we don't have a third argument, we're done. */
	if (nfargs != 3) {
		free(re);
		return;
	}

	/* We need to parse the list of registers. Anything that we don't get
	 * is assumed to be -1. If we didn't match, or the match went wonky,
	 * then set the register to empty. Otherwise, fill the register
	 * with the subexpression.
	 */
	nqregs = list2arr(qregs, NSUBEXP, fargs[2], ' ');
	for (i = 0; i < nqregs; i++) {
	    if (qregs[i] && *qregs[i] && is_integer(qregs[i]))
		curq = atoi(qregs[i]);
	    else
		curq = -1;
	    if ((curq < 0) || (curq > 9))
		continue;
	    if (!mudstate.global_regs[curq]) {
		mudstate.global_regs[curq] = alloc_lbuf("fun_regmatch");
	    }
	    if (!matched || !re->startp[i] || !re->endp[i]) {
		mudstate.global_regs[curq][0] = '\0'; /* empty string */
		mudstate.glob_reg_len[curq] = 0;
	    } else {
		len = re->endp[i] - re->startp[i];
		if (len > LBUF_SIZE - 1)
		    len = LBUF_SIZE - 1;
		else if (len < 0)
		    len = 0;
		strncpy(mudstate.global_regs[curq], re->startp[i], len);
		mudstate.global_regs[curq][len] = '\0';	/* must null-term */
		mudstate.glob_reg_len[curq] = len;
	    }
	}

	free(re);
}

/* ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second argument
 * is 0 or s, control characters are converted to spaces. If it's 1 or p,
 * they're converted to percent substitutions.
 */

FUNCTION(fun_translate)
{
    if (*fargs[0] && *fargs[1]) {

	/* Strictly speaking, we're just checking the first char */

	if (fargs[1][0] == 's')
	    safe_str(translate_string(fargs[0], 0), buff, bufc);
	else if (fargs[1][0] == 'p')
	    safe_str(translate_string(fargs[0], 1), buff, bufc);
	else
	    safe_str(translate_string(fargs[0], atoi(fargs[1])), buff, bufc);
    }
}

/* ---------------------------------------------------------------------------
 * fun_lastcreate: Return the last object of type Y that X created.
 */

FUNCTION(fun_lastcreate)
{
    int i, aowner, aflags, alen, obj_list[4], obj_type;
    char *obj_str, *p;
    dbref obj = match_thing(player, fargs[0]);

    if (!controls(player, obj)) {    /* Automatically checks for GoodObj */
	safe_nothing(buff, bufc);
	return;
    }

    switch (*fargs[1]) {
      case 'R':
      case 'r':
	obj_type = 0;
	break;
      case 'E':
      case 'e':
	obj_type = 1;;
	break;
      case 'T':
      case 't':
	obj_type = 2;
	break;
      case 'P':
      case 'p':
	obj_type = 3;
	break;
      default:
	notify_quiet(player, "Invalid object type.");
	safe_nothing(buff, bufc);
	return;
    }

    obj_str = atr_get(obj, A_NEWOBJS, &aowner, &aflags, &alen);
    if (!*obj_str) {
	free_lbuf(obj_str);
	safe_nothing(buff, bufc);
	return;
    }

    for (p = (char *) strtok(obj_str, " "), i = 0;
	 p && (i < 4);
	 p = (char *) strtok(NULL, " "), i++) {
	obj_list[i] = atoi(p);
    }
    free_lbuf(obj_str);

    *(*bufc)++ = '#';
    safe_ltos(buff, bufc, obj_list[obj_type]);
}

/*---------------------------------------------------------------------------
 * encrypt() and decrypt(): From DarkZone.
 */

/*
 * Copy over only alphanumeric chars 
 */
static char *crunch_code(code)
char *code;
{
	char *in;
	char *out;
	static char output[LBUF_SIZE];

	out = output;
	in = code;
	while (*in) {
		if ((*in >= 32) || (*in <= 126)) {
			printf("%c", *in);
			*out++ = *in;
		}
		in++;
	}
	*out = '\0';
	return (output);
}

static char *crypt_code(code, text, type)
char *code;
char *text;
int type;
{
	static char textbuff[LBUF_SIZE];
	char codebuff[LBUF_SIZE];
	int start = 32;
	int end = 126;
	int mod = end - start + 1;
	char *p, *q, *r;

	if (!text && !*text)
		return ((char *)"");
	StringCopy(codebuff, crunch_code(code));
	if (!code || !*code || !codebuff || !*codebuff)
		return (text);
	StringCopy(textbuff, "");

	p = text;
	q = codebuff;
	r = textbuff;

	/*
	 * Encryption: Simply go through each character of the text, get its
	 * ascii value, subtract start, add the ascii value (less
	 * start) of the code, mod the result, add start. Continue
	 */
	while (*p) {
		if ((*p < start) || (*p > end)) {
			p++;
			continue;
		}
		if (type)
			*r++ = (((*p++ - start) + (*q++ - start)) % mod) + start;
		else
			*r++ = (((*p++ - *q++) + 2 * mod) % mod) + start;
		if (!*q)
			q = codebuff;
	}
	*r = '\0';
	return (textbuff);
}

FUNCTION(fun_encrypt)
{
	safe_str(crypt_code(fargs[1], fargs[0], 1), buff, bufc);
}

FUNCTION(fun_decrypt)
{
	safe_str(crypt_code(fargs[1], fargs[0], 0), buff, bufc);
}

/*---------------------------------------------------------------------------
 * SQL stuff.
 */

FUNCTION(fun_sql)
{
    char row_delim, field_delim;

    /* Special -- the last two arguments are output delimiters */

    if (!fn_range_check("SQL", nfargs, 1, 3, buff, bufc))
	return;
    if (!delim_check(fargs, nfargs, 2, &row_delim, buff, bufc, 0,
		     player, cause, cargs, ncargs, 1))
	return;
    if (nfargs < 3)
	field_delim = row_delim;
    else if (!delim_check(fargs, nfargs, 3, &field_delim, buff, bufc, 0,
			  player, cause, cargs, ncargs, 1))
	return;

    sql_query(player, fargs[0], buff, bufc, row_delim, field_delim);
}

/*---------------------------------------------------------------------------
 * Pueblo HTML-related functions.
 */

#ifdef PUEBLO_SUPPORT

FUNCTION(fun_html_escape)
{
    html_escape(fargs[0], buff, bufc);
}

FUNCTION(fun_html_unescape)
{
    const char *msg_orig;
    int ret = 0;

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++) {
	switch (*msg_orig) {
	  case '&':
	    if (!strncmp(msg_orig, "&quot;", 6)) {
		ret = safe_chr_fn('\"', buff, bufc);
		msg_orig += 5;
	    } else if (!strncmp(msg_orig, "&lt;", 4)) {
		ret = safe_chr_fn('<', buff, bufc);
		msg_orig += 3;
	    } else if (!strncmp(msg_orig, "&gt;", 4)) {
		ret = safe_chr_fn('>', buff, bufc);
		msg_orig += 3;
	    } else if (!strncmp(msg_orig, "&amp;", 5)) {
		ret = safe_chr_fn('&', buff, bufc);
		msg_orig += 4;
	    }
	    break;
	  default:
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	    break;
	}
    }
}

FUNCTION(fun_url_escape)
{
    /* These are the characters which are converted to %<hex> */
    char *escaped_chars = "<>#%{}|\\^~[]';/?:@=&\"+";
    const char *msg_orig;
    int ret = 0;
    char tbuf[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++) {
	if (index(escaped_chars, *msg_orig)) {
	    sprintf(tbuf, "%%%2x", *msg_orig);
	    ret = safe_str(tbuf, buff, bufc);
	} else if (*msg_orig == ' ') {
	    ret = safe_chr_fn('+', buff, bufc);
	} else{
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	}
    }
}

FUNCTION(fun_url_unescape)
{
    const char *msg_orig;
    int ret = 0;
    unsigned int tempchar;
    char tempstr[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret;) {
	switch (*msg_orig) {
	  case '+':
	    ret = safe_chr_fn(' ', buff, bufc);
	    msg_orig++;
	    break;
	  case '%':
	    strncpy(tempstr, msg_orig+1, 2);
	    tempstr[2] = '\0';
	    if ((sscanf(tempstr, "%x", &tempchar) == 1) &&
		(tempchar > 0x1F) && (tempchar < 0x7F)) {
		ret = safe_chr_fn(tempchar, buff, bufc);
	    }
	    if (*msg_orig)
		msg_orig++;	/* Skip the '%' */
	    if (*msg_orig) 	/* Skip the 1st hex character. */
		msg_orig++;
	    if (*msg_orig)	/* Skip the 2nd hex character. */
		msg_orig++;
	    break;
	  default:
	    ret = safe_chr_fn(*msg_orig, buff, bufc);
	    msg_orig++;
	    break;
	}
    }
    return;
}
#endif /* PUEBLO_SUPPORT */
