/* functions.c - MUSH function handlers */
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
#include "externs.h"	/* required by interface */

#include "command.h"	/* required by code */
#include "match.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "fnproto.h"	/* required by code */

UFUN *ufun_head;

extern char space_buffer[LBUF_SIZE];

extern void FDECL(cf_log_notfound, (dbref player, char *cmd,
				    const char *thingname, char *thing));

/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * flist: List of existing functions in alphabetical order.
 */

FUN flist[] = {
{"ABS",		fun_abs,	1,  0,		CA_PUBLIC},
{"ACOS",	fun_acos,	1,  0,		CA_PUBLIC},
{"ADD",		fun_add,	0,  FN_VARARGS,	CA_PUBLIC},
{"AFTER",	fun_after,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMAX",	fun_alphamax,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMIN",	fun_alphamin,   0,  FN_VARARGS, CA_PUBLIC},
{"AND",		fun_and,	0,  FN_VARARGS, CA_PUBLIC},
{"ANDBOOL",	fun_andbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"ANDFLAGS",	fun_andflags,	2,  0,		CA_PUBLIC},
{"ANSI",        fun_ansi,       2,  0,          CA_PUBLIC},
{"APOSS",	fun_aposs,	1,  0,		CA_PUBLIC},
{"ART",		fun_art,	1,  0,		CA_PUBLIC},
{"ASIN",	fun_asin,	1,  0,		CA_PUBLIC},
{"ATAN",	fun_atan,	1,  0,		CA_PUBLIC},
{"BAND",	fun_band,	2,  0,		CA_PUBLIC},
{"BEEP",        fun_beep,       0,  0,          CA_WIZARD},
{"BEFORE",	fun_before,	0,  FN_VARARGS,	CA_PUBLIC},
{"BNAND",	fun_bnand,	2,  0,		CA_PUBLIC},
{"BOR",		fun_bor,	2,  0,		CA_PUBLIC},
{"CAPSTR",	fun_capstr,	-1, 0,		CA_PUBLIC},
{"CASE",	fun_case,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"CAT",		fun_cat,	0,  FN_VARARGS,	CA_PUBLIC},
{"CEIL",	fun_ceil,	1,  0,		CA_PUBLIC},
{"CENTER",	fun_center,	0,  FN_VARARGS,	CA_PUBLIC},
{"CHILDREN",    fun_children,   1,  0,          CA_PUBLIC},
{"CHOMP",	fun_chomp,	1,  0,		CA_PUBLIC},
{"CLEARVARS",	fun_clearvars,	0,  0,		CA_PUBLIC},
{"COLUMNS",	fun_columns,	0,  FN_VARARGS, CA_PUBLIC},
{"COMMAND",	fun_command,	0,  FN_VARARGS, CA_PUBLIC},
{"COMP",	fun_comp,	2,  0,		CA_PUBLIC},
{"CON",		fun_con,	1,  0,		CA_PUBLIC},
{"CONFIG",	fun_config,	1,  0,		CA_PUBLIC},
{"CONN",	fun_conn,	1,  0,		CA_PUBLIC},
{"CONSTRUCT",	fun_construct,	0,  FN_VARARGS,	CA_PUBLIC},
{"CONTROLS", 	fun_controls,	2,  0,		CA_PUBLIC},
{"CONVSECS",    fun_convsecs,   1,  0,		CA_PUBLIC},
{"CONVTIME",    fun_convtime,   1,  0,		CA_PUBLIC},
{"COS",		fun_cos,	1,  0,		CA_PUBLIC},
{"CREATE",      fun_create,     0,  FN_VARARGS, CA_PUBLIC},
#ifdef USE_COMSYS
{"CWHO",        fun_cwho,       1,  0,          CA_PUBLIC},
#endif
{"DEC",         fun_dec,        1,  0,          CA_PUBLIC},
{"DECRYPT",	fun_decrypt,	2,  0,		CA_PUBLIC},
{"DEFAULT",	fun_default,	2,  FN_NO_EVAL, CA_PUBLIC},
{"DELETE",	fun_delete,	3,  0,		CA_PUBLIC},
{"DESTRUCT",	fun_destruct,	1,  0,		CA_PUBLIC},
{"DIE",		fun_die,	2,  0,		CA_PUBLIC},
{"DIST2D",	fun_dist2d,	4,  0,		CA_PUBLIC},
{"DIST3D",	fun_dist3d,	6,  0,		CA_PUBLIC},
{"DIV",		fun_div,	2,  0,		CA_PUBLIC},
{"DOING",	fun_doing,	1,  0,		CA_PUBLIC},
{"DUP",		fun_dup,	0,  FN_VARARGS,	CA_PUBLIC},
{"E",		fun_e,		0,  0,		CA_PUBLIC},
{"EDEFAULT",	fun_edefault,	2,  FN_NO_EVAL, CA_PUBLIC},
{"EDIT",	fun_edit,	3,  0,		CA_PUBLIC},
{"ELEMENTS",	fun_elements,	0,  FN_VARARGS,	CA_PUBLIC},
{"ELOCK",	fun_elock,	2,  0,		CA_PUBLIC},
{"EMPTY",	fun_empty,	0,  FN_VARARGS, CA_PUBLIC},
{"ENCRYPT",	fun_encrypt,	2,  0,		CA_PUBLIC},
{"EQ",		fun_eq,		2,  0,		CA_PUBLIC},
{"ESCAPE",	fun_escape,	-1, 0,		CA_PUBLIC},
{"EXIT",	fun_exit,	1,  0,		CA_PUBLIC},
{"EXP",		fun_exp,	1,  0,		CA_PUBLIC},
{"EXTRACT",	fun_extract,	0,  FN_VARARGS,	CA_PUBLIC},
{"EVAL",        fun_eval,       0,  FN_VARARGS, CA_PUBLIC},
{"SUBEVAL",  	fun_subeval,	1,  0,		CA_PUBLIC},
{"FDIV",	fun_fdiv,	2,  0,		CA_PUBLIC},
{"FILTER",	fun_filter,	0,  FN_VARARGS,	CA_PUBLIC},
{"FILTERBOOL",	fun_filterbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"FINDABLE",	fun_findable,	2,  0,		CA_PUBLIC},
{"FIRST",	fun_first,	0,  FN_VARARGS,	CA_PUBLIC},
{"FLAGS",	fun_flags,	1,  0,		CA_PUBLIC},
{"FLOOR",	fun_floor,	1,  0,		CA_PUBLIC},
{"FOLD",	fun_fold,	0,  FN_VARARGS,	CA_PUBLIC},
{"FORCE",	fun_force,	2,  0,		CA_PUBLIC},
{"FOREACH",	fun_foreach,	0,  FN_VARARGS,	CA_PUBLIC},
{"FULLNAME",	fun_fullname,	1,  0,		CA_PUBLIC},
{"GET",		fun_get,	1,  0,		CA_PUBLIC},
{"GET_EVAL",	fun_get_eval,	1,  0,		CA_PUBLIC},
{"GRAB",	fun_grab,	0,  FN_VARARGS,	CA_PUBLIC},
{"GREP",	fun_grep,	3,  0,		CA_PUBLIC},
{"GREPI",	fun_grepi,	3,  0,		CA_PUBLIC},
{"GT",		fun_gt,		2,  0,		CA_PUBLIC},
{"GTE",		fun_gte,	2,  0,		CA_PUBLIC},
{"HASATTR",	fun_hasattr,	2,  0,		CA_PUBLIC},
{"HASATTRP",	fun_hasattrp,	2,  0,		CA_PUBLIC},
{"HASFLAG",	fun_hasflag,	2,  0,		CA_PUBLIC},
{"HASPOWER",    fun_haspower,   2,  0,          CA_PUBLIC},
{"HASTYPE",	fun_hastype,	2,  0,		CA_PUBLIC},
{"HOME",	fun_home,	1,  0,		CA_PUBLIC},
#ifdef PUEBLO_SUPPORT
{"HTML_ESCAPE",	fun_html_escape,-1, 0,		CA_PUBLIC},
{"HTML_UNESCAPE",fun_html_unescape,-1,0,	CA_PUBLIC},
#endif /* PUEBLO_SUPPORT */
{"IDLE",	fun_idle,	1,  0,		CA_PUBLIC},
{"IFELSE",      fun_ifelse,     3,  FN_NO_EVAL, CA_PUBLIC},
{"ILEV",	fun_ilev,	0,  0,		CA_PUBLIC},
{"INC",         fun_inc,        1,  0,          CA_PUBLIC},
{"INDEX",	fun_index,	4,  0,		CA_PUBLIC},
{"INSERT",	fun_insert,	0,  FN_VARARGS,	CA_PUBLIC},
{"INUM",	fun_inum,	1,  0,		CA_PUBLIC},
{"INZONE",      fun_inzone,     1,  0,          CA_PUBLIC},
{"ISDBREF",	fun_isdbref,	1,  0,		CA_PUBLIC},
{"ISNUM",	fun_isnum,	1,  0,		CA_PUBLIC},
{"ISWORD",	fun_isword,	1,  0,		CA_PUBLIC},
{"ITEMS",	fun_items,	0,  FN_VARARGS,	CA_PUBLIC},
{"ITER",	fun_iter,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"ITEXT",	fun_itext,	1,  0,		CA_PUBLIC},
{"LADD",	fun_ladd,	0,  FN_VARARGS,	CA_PUBLIC},
{"LAND",	fun_land,	0,  FN_VARARGS,	CA_PUBLIC},
{"LANDBOOL",	fun_landbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"LAST",	fun_last,	0,  FN_VARARGS,	CA_PUBLIC},
{"LASTCREATE",	fun_lastcreate,	2,  0,		CA_PUBLIC},
{"LATTR",	fun_lattr,	1,  0,		CA_PUBLIC},
{"LCON",	fun_lcon,	1,  0,		CA_PUBLIC},
{"LCSTR",	fun_lcstr,	-1, 0,		CA_PUBLIC},
{"LDELETE",	fun_ldelete,	0,  FN_VARARGS,	CA_PUBLIC},
{"LEDIT",	fun_ledit,	0,  FN_VARARGS, CA_PUBLIC},
{"LEFT",	fun_left,	2,  0,		CA_PUBLIC},
{"LET",		fun_let,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LEXITS",	fun_lexits,	1,  0,		CA_PUBLIC},
{"LIST",	fun_list,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC}, 
{"LIT",		fun_lit,	1,  FN_NO_EVAL,	CA_PUBLIC},
{"LJUST",	fun_ljust,	0,  FN_VARARGS,	CA_PUBLIC},
{"LINK",	fun_link,	2,  0,		CA_PUBLIC},
{"LINSTANCES",	fun_linstances,	0,  0,		CA_PUBLIC},
{"LMAX",	fun_lmax,	0,  FN_VARARGS,	CA_PUBLIC},
{"LMIN",	fun_lmin,	0,  FN_VARARGS,	CA_PUBLIC},
{"LN",		fun_ln,		1,  0,		CA_PUBLIC},
{"LNUM",	fun_lnum,	0,  FN_VARARGS,	CA_PUBLIC},
{"LOAD",	fun_load,	0,  FN_VARARGS,	CA_PUBLIC},
{"LOC",		fun_loc,	1,  0,		CA_PUBLIC},
{"LOCATE",	fun_locate,	3,  0,		CA_PUBLIC},
{"LOCALIZE",    fun_localize,   1,  FN_NO_EVAL, CA_PUBLIC},
{"LOCK",	fun_lock,	1,  0,		CA_PUBLIC},
{"LOG",		fun_log,	0,  FN_VARARGS,	CA_PUBLIC},
{"LPARENT",	fun_lparent,	1,  0,		CA_PUBLIC}, 
{"LOOP",	fun_loop,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LOR",		fun_lor,	0,  FN_VARARGS,	CA_PUBLIC},
{"LORBOOL",	fun_lorbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"LPOS",	fun_lpos,	2,  0,		CA_PUBLIC},
{"LRAND",	fun_lrand,	0,  FN_VARARGS, CA_PUBLIC},
{"LSTACK",	fun_lstack,	0,  FN_VARARGS, CA_PUBLIC},
{"LSTRUCTURES",	fun_lstructures, 0, 0,		CA_PUBLIC},
{"LT",		fun_lt,		2,  0,		CA_PUBLIC},
{"LTE",		fun_lte,	2,  0,		CA_PUBLIC},
{"LVARS",	fun_lvars,	0,  0,		CA_PUBLIC},
{"LWHO",	fun_lwho,	0,  0,		CA_PUBLIC},
#ifdef USE_MAIL
{"MAIL",        fun_mail,       0,  FN_VARARGS, CA_PUBLIC},
{"MAILFROM",    fun_mailfrom,   0,  FN_VARARGS, CA_PUBLIC},
#endif
{"MAP",		fun_map,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCH",	fun_match,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCHALL",	fun_matchall,	0,  FN_VARARGS,	CA_PUBLIC},
{"MAX",		fun_max,	0,  FN_VARARGS,	CA_PUBLIC},
{"MEMBER",	fun_member,	0,  FN_VARARGS,	CA_PUBLIC},
{"MERGE",	fun_merge,	3,  0,		CA_PUBLIC},
{"MID",		fun_mid,	3,  0,		CA_PUBLIC},
{"MIN",		fun_min,	0,  FN_VARARGS,	CA_PUBLIC},
{"MIX",		fun_mix,	0,  FN_VARARGS,	CA_PUBLIC},
{"MOD",		fun_mod,	2,  0,		CA_PUBLIC},
{"MODIFY",	fun_modify,	3,  0,		CA_PUBLIC},
{"MONEY",	fun_money,	1,  0,		CA_PUBLIC},
{"MUDNAME",	fun_mudname,	0,  0,		CA_PUBLIC},
{"MUL",		fun_mul,	0,  FN_VARARGS,	CA_PUBLIC},
{"MUNGE",	fun_munge,	0,  FN_VARARGS,	CA_PUBLIC},
{"NAME",	fun_name,	1,  0,		CA_PUBLIC},
{"NCOMP",	fun_ncomp,	2,  0,		CA_PUBLIC},
{"NEARBY",	fun_nearby,	2,  0,		CA_PUBLIC},
{"NEQ",		fun_neq,	2,  0,		CA_PUBLIC},
{"NEXT",	fun_next,	1,  0,		CA_PUBLIC},
{"NONZERO",	fun_nonzero,	3,  FN_NO_EVAL, CA_PUBLIC},
{"NOT",		fun_not,	1,  0,		CA_PUBLIC},
{"NOTBOOL",	fun_notbool,	1,  0,		CA_PUBLIC},
{"NULL",	fun_null,	1,  0,		CA_PUBLIC},
{"NUM",		fun_num,	1,  0,		CA_PUBLIC},
{"OBJ",		fun_obj,	1,  0,		CA_PUBLIC},
{"OBJEVAL",     fun_objeval,    2,  FN_NO_EVAL, CA_PUBLIC},
{"OBJMEM",	fun_objmem,	1,  0,		CA_PUBLIC},
{"OR",		fun_or,		0,  FN_VARARGS, CA_PUBLIC},
{"ORBOOL",	fun_orbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"ORFLAGS",	fun_orflags,	2,  0,		CA_PUBLIC},
{"OWNER",	fun_owner,	1,  0,		CA_PUBLIC},
{"PARENT",	fun_parent,	1,  0,		CA_PUBLIC},
{"PARSE",	fun_parse,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"PEEK",	fun_peek,	0,  FN_VARARGS, CA_PUBLIC},
{"PEMIT",	fun_pemit,	2,  0,		CA_PUBLIC},
{"PFIND",	fun_pfind,	1,  0,		CA_PUBLIC},
{"PI",		fun_pi,		0,  0,		CA_PUBLIC},
{"PLAYMEM",	fun_playmem,	1,  0,		CA_PUBLIC},
{"PMATCH",	fun_pmatch,	1,  0,		CA_PUBLIC},
{"POP",		fun_pop,	0,  FN_VARARGS, CA_PUBLIC},
{"POPN",	fun_popn,	0,  FN_VARARGS,	CA_PUBLIC},
{"PORTS",	fun_ports,	1,  0,		CA_PUBLIC},
{"POS",		fun_pos,	2,  0,		CA_PUBLIC},
{"POSS",	fun_poss,	1,  0,		CA_PUBLIC},
{"POWER",	fun_power,	2,  0,		CA_PUBLIC},
{"PROGRAMMER",	fun_programmer,	1,  0,		CA_PUBLIC},
{"PUSH",	fun_push,	0,  FN_VARARGS, CA_PUBLIC},
{"R",		fun_r,		1,  0,		CA_PUBLIC},
{"RAND",	fun_rand,	1,  0,		CA_PUBLIC},
{"REGMATCH",	fun_regmatch,	0,  FN_VARARGS, CA_PUBLIC},
{"REGMATCHI",	fun_regmatchi,	0,  FN_VARARGS, CA_PUBLIC},
{"REGPARSE",	fun_regparse,	3,  0,		CA_PUBLIC},
{"REGPARSEI",	fun_regparsei,	3,  0,		CA_PUBLIC},
{"REMIT",	fun_remit,	2,  0,		CA_PUBLIC},
{"REMOVE",	fun_remove,	0,  FN_VARARGS,	CA_PUBLIC},
{"REPEAT",	fun_repeat,	2,  0,		CA_PUBLIC},
{"REPLACE",	fun_replace,	0,  FN_VARARGS,	CA_PUBLIC},
{"REST",	fun_rest,	0,  FN_VARARGS,	CA_PUBLIC},
{"RESTARTS",	fun_restarts,	0,  0,		CA_PUBLIC},
{"RESTARTTIME",	fun_restarttime, 0, 0,		CA_PUBLIC},
{"REVERSE",	fun_reverse,	-1, 0,		CA_PUBLIC},
{"REVWORDS",	fun_revwords,	0,  FN_VARARGS,	CA_PUBLIC},
{"RIGHT",	fun_right,	2,  0,		CA_PUBLIC},
{"RJUST",	fun_rjust,	0,  FN_VARARGS,	CA_PUBLIC},
{"RLOC",	fun_rloc,	2,  0,		CA_PUBLIC},
{"ROOM",	fun_room,	1,  0,		CA_PUBLIC},
{"ROUND",	fun_round,	2,  0,		CA_PUBLIC},
{"S",		fun_s,		-1, 0,		CA_PUBLIC},
{"SCRAMBLE",	fun_scramble,	1,  0,		CA_PUBLIC},
{"SEARCH",	fun_search,	-1, 0,		CA_PUBLIC},
{"SECS",	fun_secs,	0,  0,		CA_PUBLIC},
{"SECURE",	fun_secure,	-1, 0,		CA_PUBLIC},
{"SEES",	fun_sees,	2,  0,		CA_PUBLIC},
{"SET",		fun_set,	2,  0,		CA_PUBLIC},
{"SETDIFF",	fun_setdiff,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETINTER",	fun_setinter,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETQ",	fun_setq,	2,  0,		CA_PUBLIC},
{"SETR",	fun_setr,	2,  0,		CA_PUBLIC},
{"SETX",	fun_setx,	2,  0,		CA_PUBLIC},
{"SETXR",	fun_setxr,	2,  0,		CA_PUBLIC},
{"SETUNION",	fun_setunion,	0,  FN_VARARGS,	CA_PUBLIC},
{"SHL",		fun_shl,	2,  0,		CA_PUBLIC},
{"SHR",		fun_shr,	2,  0,		CA_PUBLIC},
{"SHUFFLE",	fun_shuffle,	0,  FN_VARARGS,	CA_PUBLIC},
{"SIGN",	fun_sign,	1,  0,		CA_PUBLIC},
{"SIN",		fun_sin,	1,  0,		CA_PUBLIC},
{"SORT",	fun_sort,	0,  FN_VARARGS,	CA_PUBLIC},
{"SORTBY",	fun_sortby,	0,  FN_VARARGS, CA_PUBLIC},
{"SPACE",	fun_space,	1,  0,		CA_PUBLIC},
{"SPLICE",	fun_splice,	0,  FN_VARARGS,	CA_PUBLIC},
{"SQL",		fun_sql,	0,  FN_VARARGS, CA_SQL_OK},
{"SQRT",	fun_sqrt,	1,  0,		CA_PUBLIC},
{"SQUISH",	fun_squish,	0,  FN_VARARGS,	CA_PUBLIC},
{"STARTTIME",	fun_starttime,	0,  0,		CA_PUBLIC},
{"STATS",	fun_stats,	1,  0,		CA_PUBLIC},
{"STEP",	fun_step,	0,  FN_VARARGS, CA_PUBLIC},
{"STRCAT",	fun_strcat,	0,  FN_VARARGS,	CA_PUBLIC},
{"STREQ",	fun_streq,	2,  0,		CA_PUBLIC},
{"STRIPANSI",	fun_stripansi,	1,  0,		CA_PUBLIC},
{"STRLEN",	fun_strlen,	-1, 0,		CA_PUBLIC},
{"STRMATCH",	fun_strmatch,	2,  0,		CA_PUBLIC},
{"STRTRUNC",    fun_strtrunc,   2,  0,          CA_PUBLIC},
{"STRUCTURE",	fun_structure,	0,  FN_VARARGS,	CA_PUBLIC},
{"SUB",		fun_sub,	2,  0,		CA_PUBLIC},
{"SUBJ",	fun_subj,	1,  0,		CA_PUBLIC},
{"SWAP",	fun_swap,	0,  FN_VARARGS,	CA_PUBLIC},
{"SWITCH",	fun_switch,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"SWITCHALL",	fun_switchall,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"T",		fun_t,		1,  0,		CA_PUBLIC},
{"TAN",		fun_tan,	1,  0,		CA_PUBLIC},
{"TEL",		fun_tel,	2,  0,		CA_PUBLIC},
{"TIME",	fun_time,	0,  0,		CA_PUBLIC},
{"TOSS",	fun_toss,	0,  FN_VARARGS, CA_PUBLIC},
{"TRANSLATE",	fun_translate,	2,  0,		CA_PUBLIC},
{"TRIGGER",	fun_trigger,	0,  FN_VARARGS, CA_PUBLIC},
{"TRIM",	fun_trim,	0,  FN_VARARGS,	CA_PUBLIC},
{"TRUNC",	fun_trunc,	1,  0,		CA_PUBLIC},
{"TYPE",	fun_type,	1,  0,		CA_PUBLIC},
{"U",		fun_u,		0,  FN_VARARGS,	CA_PUBLIC},
{"UCSTR",	fun_ucstr,	-1, 0,		CA_PUBLIC},
{"UDEFAULT",	fun_udefault,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"ULOCAL",	fun_ulocal,	0,  FN_VARARGS,	CA_PUBLIC},
{"UNLOAD",	fun_unload,	0,  FN_VARARGS,	CA_PUBLIC},
{"UNSTRUCTURE",	fun_unstructure,1,  0,		CA_PUBLIC},
{"UNTIL",	fun_until,	0,  FN_VARARGS, CA_PUBLIC},
#ifdef PUEBLO_SUPPORT
{"URL_ESCAPE",	fun_url_escape,	-1, 0,		CA_PUBLIC},
{"URL_UNESCAPE",fun_url_unescape,-1,0,		CA_PUBLIC},
#endif /* PUEBLO_SUPPORT */
{"V",		fun_v,		1,  0,		CA_PUBLIC},
{"VADD",	fun_vadd,	0,  FN_VARARGS,	CA_PUBLIC},
{"VALID",	fun_valid,	2,  FN_VARARGS, CA_PUBLIC},
{"VDIM",	fun_vdim,	0,  FN_VARARGS,	CA_PUBLIC},
{"VDOT",	fun_vdot,	0,  FN_VARARGS,	CA_PUBLIC},
{"VERSION",	fun_version,	0,  0,		CA_PUBLIC},
{"VISIBLE",	fun_visible,	2,  0,		CA_PUBLIC},
{"VMAG",	fun_vmag,	0,  FN_VARARGS,	CA_PUBLIC},
{"VMUL",	fun_vmul,	0,  FN_VARARGS,	CA_PUBLIC},
{"VSUB",	fun_vsub,	0,  FN_VARARGS,	CA_PUBLIC},
{"VUNIT",	fun_vunit,	0,  FN_VARARGS,	CA_PUBLIC},
{"WAIT",	fun_wait,	2,  0,		CA_PUBLIC},
{"WHENFALSE",	fun_whenfalse,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"WHENTRUE",	fun_whentrue,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"WHERE",	fun_where,	1,  0,		CA_PUBLIC},
{"WHILE",	fun_while,	0,  FN_VARARGS,	CA_PUBLIC},
{"WIPE",	fun_wipe,	1,  0,		CA_PUBLIC},
{"WORDPOS",     fun_wordpos,    0,  FN_VARARGS,	CA_PUBLIC},
{"WORDS",	fun_words,	0,  FN_VARARGS,	CA_PUBLIC},
{"X",		fun_x,		1,  0,		CA_PUBLIC},
{"XCON",	fun_xcon,	3,  0,		CA_PUBLIC},
{"XGET",	fun_xget,	2,  0,		CA_PUBLIC},
{"XOR",		fun_xor,	0,  FN_VARARGS,	CA_PUBLIC},
{"XORBOOL",	fun_xorbool,	0,  FN_VARARGS,	CA_PUBLIC},
{"XVARS",	fun_xvars,	0,  FN_VARARGS, CA_PUBLIC},
{"Z",		fun_z,		2,  0,		CA_PUBLIC},
{"ZFUN",	fun_zfun,	0,  FN_VARARGS,	CA_PUBLIC},
{"ZONE",        fun_zone,       1,  0,          CA_PUBLIC},
{"ZWHO",        fun_zwho,       1,  0,          CA_PUBLIC},
{NULL,		NULL,		0,  0,		0}};

/* *INDENT-ON* */

void NDECL(init_functab)
{
	FUN *fp;
	char *buff, *cp, *dp;
	int i;

	buff = alloc_sbuf("init_functab");
	hashinit(&mudstate.func_htab, 250 * HASH_FACTOR);
	for (fp = flist; fp->name; fp++) {
		cp = (char *)fp->name;
		dp = buff;
		while (*cp) {
			*dp = ToLower(*cp);
			cp++;
			dp++;
		}
		*dp = '\0';
		hashadd(buff, (int *)fp, &mudstate.func_htab);
	}
	free_sbuf(buff);
	ufun_head = NULL;
	hashinit(&mudstate.ufunc_htab, 15 * HASH_FACTOR);

	/* Initialize the space table, which is used for fast copies on
	 * functions like ljust().
	 */
	for (i = 0; i < LBUF_SIZE; i++)
	    space_buffer[i] = ' ';
	space_buffer[LBUF_SIZE - 1] = '\0';
}

void do_function(player, cause, key, fname, target)
dbref player, cause;
int key;
char *fname, *target;
{
	UFUN *ufp, *ufp2;
	ATTR *ap;
	char *np, *bp;
	int atr, aflags;
	dbref obj, aowner;

	/* Make a local uppercase copy of the function name */

	bp = np = alloc_sbuf("add_user_func");
	safe_sb_str(fname, np, &bp);
	*bp = '\0';
	for (bp = np; *bp; bp++)
		*bp = ToLower(*bp);

	/* Verify that the function doesn't exist in the builtin table */

	if (hashfind(np, &mudstate.func_htab) != NULL) {
		notify_quiet(player,
		     "Function already defined in builtin function table.");
		free_sbuf(np);
		return;
	}
	/* Make sure the target object exists */

	if (!parse_attrib(player, target, &obj, &atr)) {
		notify_quiet(player, NOMATCH_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* Make sure the attribute exists */

	if (atr == NOTHING) {
		notify_quiet(player, "No such attribute.");
		free_sbuf(np);
		return;
	}
	/* Make sure attribute is readably by me */

	ap = atr_num(atr);
	if (!ap) {
		notify_quiet(player, "No such attribute.");
		free_sbuf(np);
		return;
	}
	atr_get_info(obj, atr, &aowner, &aflags);
	if (!See_attr(player, obj, ap, aowner, aflags)) {
		notify_quiet(player, NOPERM_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* Privileged functions require you control the obj.  */

	if ((key & FN_PRIV) && !Controls(player, obj)) {
		notify_quiet(player, NOPERM_MESSAGE);
		free_sbuf(np);
		return;
	}
	/* See if function already exists.  If so, redefine it */

	ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab);

	if (!ufp) {
		ufp = (UFUN *) XMALLOC(sizeof(UFUN), "do_function");
		ufp->name = strsave(np);
		for (bp = (char *)ufp->name; *bp; bp++)
			*bp = ToUpper(*bp);
		ufp->obj = obj;
		ufp->atr = atr;
		ufp->perms = CA_PUBLIC;
		ufp->next = NULL;
		if (!ufun_head) {
			ufun_head = ufp;
		} else {
			for (ufp2 = ufun_head; ufp2->next; ufp2 = ufp2->next) ;
			ufp2->next = ufp;
		}
		if (hashadd(np, (int *) ufp, &mudstate.ufunc_htab)) {
			notify_quiet(player, tprintf("Function %s not defined.", fname));
			XFREE(ufp->name, "do_function");
			XFREE(ufp, "do_function.2");
			free_sbuf(np);
			return;
		}
        }
	ufp->obj = obj;
	ufp->atr = atr;
	ufp->flags = key;
	free_sbuf(np);
	if (!Quiet(player))
		notify_quiet(player, tprintf("Function %s defined.", fname));
}

/* ---------------------------------------------------------------------------
 * list_functable: List available functions.
 */

void list_functable(player)
dbref player;
{
	FUN *fp;
	UFUN *ufp;
	char *buf, *bp, *cp;

	buf = alloc_lbuf("list_functable");
	bp = buf;
	for (cp = (char *)"Functions:"; *cp; cp++)
		*bp++ = *cp;
	for (fp = flist; fp->name; fp++) {
		if (check_access(player, fp->perms)) {
			*bp++ = ' ';
			for (cp = (char *)(fp->name); *cp; cp++)
				*bp++ = *cp;
		}
	}
	for (ufp = ufun_head; ufp; ufp = ufp->next) {
		if (check_access(player, ufp->perms)) {
			*bp++ = ' ';
			for (cp = (char *)(ufp->name); *cp; cp++)
				*bp++ = *cp;
		}
	}
	*bp = '\0';
	notify(player, buf);
	free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * cf_func_access: set access on functions
 */

CF_HAND(cf_func_access)
{
	FUN *fp;
	UFUN *ufp;
	char *ap;

	for (ap = str; *ap && !isspace(*ap); ap++) ;
	if (*ap)
		*ap++ = '\0';

	for (fp = flist; fp->name; fp++) {
		if (!string_compare(fp->name, str)) {
			return (cf_modify_bits(&fp->perms, ap, extra,
					       player, cmd));
		}
	}
	for (ufp = ufun_head; ufp; ufp = ufp->next) {
		if (!string_compare(ufp->name, str)) {
			return (cf_modify_bits(&ufp->perms, ap, extra,
					       player, cmd));
		}
	}
	cf_log_notfound(player, cmd, "Function", str);
	return -1;
}
