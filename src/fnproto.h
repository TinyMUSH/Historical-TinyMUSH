/* fnproto.h - function prototypes from funmath.c, etc. */
/* $Id$ */

#include "copyright.h"

#ifndef __FNPROTO_H
#define __FNPROTO_H

#define	XFUNCTION(x)	\
	extern void FDECL(x, (char *, char **, dbref, dbref, \
			      char *[], int, char *[], int))

/* From funext.c */

#ifdef USE_MAIL
XFUNCTION(fun_mail);
XFUNCTION(fun_mailfrom);
#endif

#ifdef PUEBLO_SUPPORT
XFUNCTION(fun_html_escape);
XFUNCTION(fun_html_unescape);
XFUNCTION(fun_url_escape);
XFUNCTION(fun_url_unescape);
#endif /* PUEBLO SUPPORT */

XFUNCTION(fun_config);
XFUNCTION(fun_lwho);
XFUNCTION(fun_ports);
XFUNCTION(fun_doing);
XFUNCTION(fun_idle);
XFUNCTION(fun_conn);
XFUNCTION(fun_programmer);
XFUNCTION(fun_sql);

/* From funiter.c */

XFUNCTION(fun_parse);
XFUNCTION(fun_loop);
XFUNCTION(fun_iter);
XFUNCTION(fun_whenfalse);
XFUNCTION(fun_whentrue);
XFUNCTION(fun_list);
XFUNCTION(fun_ilev);
XFUNCTION(fun_itext);
XFUNCTION(fun_inum);
XFUNCTION(fun_fold);
XFUNCTION(fun_filter);
XFUNCTION(fun_filterbool);
XFUNCTION(fun_map);
XFUNCTION(fun_mix);
XFUNCTION(fun_step);
XFUNCTION(fun_foreach);
XFUNCTION(fun_munge);
XFUNCTION(fun_while);

/* From funlist.c */

XFUNCTION(fun_words);
XFUNCTION(fun_first);
XFUNCTION(fun_rest);
XFUNCTION(fun_last);
XFUNCTION(fun_match);
XFUNCTION(fun_matchall);
XFUNCTION(fun_extract);
XFUNCTION(fun_index);
XFUNCTION(fun_ldelete);
XFUNCTION(fun_replace);
XFUNCTION(fun_insert);
XFUNCTION(fun_remove);
XFUNCTION(fun_member);
XFUNCTION(fun_revwords);
XFUNCTION(fun_splice);
XFUNCTION(fun_sort);
XFUNCTION(fun_sortby);
XFUNCTION(fun_setunion);
XFUNCTION(fun_setdiff);
XFUNCTION(fun_setinter);
XFUNCTION(fun_columns);
XFUNCTION(fun_elements);
XFUNCTION(fun_grab);
XFUNCTION(fun_graball);
XFUNCTION(fun_shuffle);
XFUNCTION(fun_ledit);

/* From funmath.c */

XFUNCTION(fun_ncomp);
XFUNCTION(fun_gt);
XFUNCTION(fun_gte);
XFUNCTION(fun_lt);
XFUNCTION(fun_lte);
XFUNCTION(fun_eq);
XFUNCTION(fun_neq);
XFUNCTION(fun_and);
XFUNCTION(fun_or);
XFUNCTION(fun_xor);
XFUNCTION(fun_not);
XFUNCTION(fun_ladd);
XFUNCTION(fun_lor);
XFUNCTION(fun_land);
XFUNCTION(fun_lorbool);
XFUNCTION(fun_landbool);
XFUNCTION(fun_lmax);
XFUNCTION(fun_lmin);
XFUNCTION(fun_andbool);
XFUNCTION(fun_orbool);
XFUNCTION(fun_xorbool);
XFUNCTION(fun_notbool);
XFUNCTION(fun_t);
XFUNCTION(fun_sqrt);
XFUNCTION(fun_add);
XFUNCTION(fun_sub);
XFUNCTION(fun_mul);
XFUNCTION(fun_floor);
XFUNCTION(fun_ceil);
XFUNCTION(fun_round);
XFUNCTION(fun_trunc);
XFUNCTION(fun_div);
XFUNCTION(fun_floordiv);
XFUNCTION(fun_fdiv);
XFUNCTION(fun_modulo);
XFUNCTION(fun_remainder);
XFUNCTION(fun_pi);
XFUNCTION(fun_e);
XFUNCTION(fun_sin);
XFUNCTION(fun_cos);
XFUNCTION(fun_tan);
XFUNCTION(fun_exp);
XFUNCTION(fun_power);
XFUNCTION(fun_ln);
XFUNCTION(fun_log);
XFUNCTION(fun_asin);
XFUNCTION(fun_acos);
XFUNCTION(fun_atan);
XFUNCTION(fun_dist2d);
XFUNCTION(fun_dist3d);
XFUNCTION(fun_vadd);
XFUNCTION(fun_vsub);
XFUNCTION(fun_vmul);
XFUNCTION(fun_vdot);
XFUNCTION(fun_vmag);
XFUNCTION(fun_vunit);
XFUNCTION(fun_vdim);
XFUNCTION(fun_abs);
XFUNCTION(fun_sign);
XFUNCTION(fun_shl);
XFUNCTION(fun_shr);
XFUNCTION(fun_band);
XFUNCTION(fun_bor);
XFUNCTION(fun_bnand);
XFUNCTION(fun_max);
XFUNCTION(fun_min);
XFUNCTION(fun_inc);
XFUNCTION(fun_dec);

/* From funmisc.c */

XFUNCTION(fun_switchall);
XFUNCTION(fun_switch);
XFUNCTION(fun_case);
XFUNCTION(fun_ifelse);
XFUNCTION(fun_nonzero);
XFUNCTION(fun_rand);
XFUNCTION(fun_die);
XFUNCTION(fun_lrand);
XFUNCTION(fun_lnum);
XFUNCTION(fun_time);
XFUNCTION(fun_secs);
XFUNCTION(fun_convsecs);
XFUNCTION(fun_convtime);
XFUNCTION(fun_starttime);
XFUNCTION(fun_restarts);
XFUNCTION(fun_restarttime);
XFUNCTION(fun_version);
XFUNCTION(fun_mudname);
XFUNCTION(fun_s);
XFUNCTION(fun_subeval);
XFUNCTION(fun_link);
XFUNCTION(fun_tel);
XFUNCTION(fun_wipe);
XFUNCTION(fun_pemit);
XFUNCTION(fun_remit);
XFUNCTION(fun_force);
XFUNCTION(fun_trigger);
XFUNCTION(fun_wait);
XFUNCTION(fun_command);
XFUNCTION(fun_create);
XFUNCTION(fun_set);

/* From funobj.c */

XFUNCTION(fun_con);
XFUNCTION(fun_exit);
XFUNCTION(fun_next);
XFUNCTION(fun_loc);
XFUNCTION(fun_where);
XFUNCTION(fun_rloc);
XFUNCTION(fun_room);
XFUNCTION(fun_owner);
XFUNCTION(fun_controls);
XFUNCTION(fun_sees);
XFUNCTION(fun_nearby);
XFUNCTION(fun_fullname);
XFUNCTION(fun_name);
XFUNCTION(fun_obj);
XFUNCTION(fun_poss);
XFUNCTION(fun_subj);
XFUNCTION(fun_aposs);
XFUNCTION(fun_lock);
XFUNCTION(fun_elock);
XFUNCTION(fun_xcon);
XFUNCTION(fun_lcon);
XFUNCTION(fun_lexits);
XFUNCTION(fun_home);
XFUNCTION(fun_money);
XFUNCTION(fun_findable);
XFUNCTION(fun_visible);
XFUNCTION(fun_flags);
XFUNCTION(fun_orflags);
XFUNCTION(fun_andflags);
XFUNCTION(fun_hasflag);
XFUNCTION(fun_haspower);
XFUNCTION(fun_parent);
XFUNCTION(fun_lparent);
XFUNCTION(fun_children);
XFUNCTION(fun_zone);
XFUNCTION(fun_zwho);
XFUNCTION(fun_inzone);
XFUNCTION(fun_zfun);
XFUNCTION(fun_hasattr);
XFUNCTION(fun_hasattrp);
XFUNCTION(fun_v);
XFUNCTION(fun_get);
XFUNCTION(fun_xget);
XFUNCTION(fun_get_eval);
XFUNCTION(fun_eval);
XFUNCTION(fun_u);
XFUNCTION(fun_ulocal);
XFUNCTION(fun_localize);
XFUNCTION(fun_default);
XFUNCTION(fun_edefault);
XFUNCTION(fun_udefault);
XFUNCTION(fun_objeval);
XFUNCTION(fun_num);
XFUNCTION(fun_pmatch);
XFUNCTION(fun_pfind);
XFUNCTION(fun_locate);
XFUNCTION(fun_lattr);
XFUNCTION(fun_search);
XFUNCTION(fun_stats);
XFUNCTION(fun_objmem);
XFUNCTION(fun_playmem);
XFUNCTION(fun_type);
XFUNCTION(fun_hastype);
XFUNCTION(fun_lastcreate);

/* From funstring.c */

XFUNCTION(fun_isword);
XFUNCTION(fun_isnum);
XFUNCTION(fun_isdbref);
XFUNCTION(fun_null);
XFUNCTION(fun_squish);
XFUNCTION(fun_trim);
XFUNCTION(fun_after);
XFUNCTION(fun_before);
XFUNCTION(fun_lcstr);
XFUNCTION(fun_ucstr);
XFUNCTION(fun_capstr);
XFUNCTION(fun_space);
XFUNCTION(fun_ljust);
XFUNCTION(fun_rjust);
XFUNCTION(fun_center);
XFUNCTION(fun_left);
XFUNCTION(fun_right);
XFUNCTION(fun_strtrunc);
XFUNCTION(fun_chomp);
XFUNCTION(fun_comp);
XFUNCTION(fun_streq);
XFUNCTION(fun_strmatch);
XFUNCTION(fun_edit);
XFUNCTION(fun_merge);
XFUNCTION(fun_secure);
XFUNCTION(fun_escape);
XFUNCTION(fun_ansi);
XFUNCTION(fun_stripansi);
XFUNCTION(fun_encrypt);
XFUNCTION(fun_decrypt);
XFUNCTION(fun_scramble);
XFUNCTION(fun_reverse);
XFUNCTION(fun_mid);
XFUNCTION(fun_translate);
XFUNCTION(fun_pos);
XFUNCTION(fun_lpos);
XFUNCTION(fun_wordpos);
XFUNCTION(fun_repeat);
XFUNCTION(fun_cat);
XFUNCTION(fun_strcat);
XFUNCTION(fun_strlen);
XFUNCTION(fun_delete);
XFUNCTION(fun_lit);
XFUNCTION(fun_art);
XFUNCTION(fun_alphamax);
XFUNCTION(fun_alphamin);
XFUNCTION(fun_valid);
XFUNCTION(fun_beep);
XFUNCTION(fun_grep);
XFUNCTION(fun_grepi);

/* From funvars.c */

XFUNCTION(fun_setq);
XFUNCTION(fun_setr);
XFUNCTION(fun_r);
XFUNCTION(fun_wildmatch);
XFUNCTION(fun_x);
XFUNCTION(fun_setx);
XFUNCTION(fun_store);
XFUNCTION(fun_xvars);
XFUNCTION(fun_let);
XFUNCTION(fun_lvars);
XFUNCTION(fun_clearvars);
XFUNCTION(fun_structure);
XFUNCTION(fun_construct);
XFUNCTION(fun_load);
XFUNCTION(fun_z);
XFUNCTION(fun_modify);
XFUNCTION(fun_unload);
XFUNCTION(fun_destruct);
XFUNCTION(fun_unstructure);
XFUNCTION(fun_lstructures);
XFUNCTION(fun_linstances);
XFUNCTION(fun_empty);
XFUNCTION(fun_items);
XFUNCTION(fun_push);
XFUNCTION(fun_dup);
XFUNCTION(fun_swap);
XFUNCTION(fun_pop);
XFUNCTION(fun_toss);
XFUNCTION(fun_popn);
XFUNCTION(fun_peek);
XFUNCTION(fun_lstack);
XFUNCTION(fun_wildparse);
XFUNCTION(fun_regparse);
XFUNCTION(fun_regparsei);
XFUNCTION(fun_regmatch);
XFUNCTION(fun_regmatchi);
XFUNCTION(fun_until);

/* From comsys.c */

#ifdef USE_COMSYS
XFUNCTION(fun_cwho);
XFUNCTION(fun_cwhoall);
XFUNCTION(fun_comlist);
XFUNCTION(fun_comowner);
XFUNCTION(fun_comdesc);
XFUNCTION(fun_comalias);
XFUNCTION(fun_cominfo);
XFUNCTION(fun_comtitle);
#endif /* USE_COMSYS */

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
#ifdef USE_COMSYS
{"COMALIAS",	fun_comalias,	1,  0,		CA_PUBLIC},
{"COMDESC",	fun_comdesc,	1,  0,		CA_PUBLIC},
{"COMINFO",	fun_cominfo,	2,  0,		CA_PUBLIC},
{"COMLIST",	fun_comlist,	0,  FN_VARARGS, CA_PUBLIC},
{"COMOWNER",	fun_comowner,	1,  0,		CA_PUBLIC},
{"COMTITLE",	fun_comtitle,	2,  0,		CA_PUBLIC},
#endif /* USE_COMSYS */
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
{"CWHOALL",     fun_cwhoall,    1,  0,          CA_PUBLIC},
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
{"FLOORDIV",	fun_floordiv,	2,  0,		CA_PUBLIC},
{"FOLD",	fun_fold,	0,  FN_VARARGS,	CA_PUBLIC},
{"FORCE",	fun_force,	2,  0,		CA_PUBLIC},
{"FOREACH",	fun_foreach,	0,  FN_VARARGS,	CA_PUBLIC},
{"FULLNAME",	fun_fullname,	1,  0,		CA_PUBLIC},
{"GET",		fun_get,	1,  0,		CA_PUBLIC},
{"GET_EVAL",	fun_get_eval,	1,  0,		CA_PUBLIC},
{"GRAB",	fun_grab,	0,  FN_VARARGS,	CA_PUBLIC},
{"GRABALL",	fun_graball,	0,  FN_VARARGS,	CA_PUBLIC},
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
{"MODULO",	fun_modulo,	2,  0,		CA_PUBLIC},
{"MODIFY",	fun_modify,	0,  FN_VARARGS,	CA_PUBLIC},
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
{"REMAINDER",	fun_remainder,	2,  0,		CA_PUBLIC},
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
{"STORE",	fun_store,	2,  0,		CA_PUBLIC},
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
{"WILDMATCH",	fun_wildmatch,	3,  0,		CA_PUBLIC},
{"WILDPARSE",	fun_wildparse,	3,  0,		CA_PUBLIC},
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

#endif /* __FNPROTO_H */
