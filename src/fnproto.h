/* fnproto.h - function prototypes from funmath.c, etc. */
/* $Id$ */

#include "copyright.h"

#ifndef __FNPROTO_H
#define __FNPROTO_H

#define	XFUNCTION(x)	\
	extern void FDECL(x, (char *, char **, dbref, dbref, dbref, \
			      char *[], int, char *[], int))

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
XFUNCTION(handle_conninfo);
XFUNCTION(fun_conn);
XFUNCTION(fun_session);
XFUNCTION(fun_programmer);
XFUNCTION(fun_helptext);
XFUNCTION(fun_sql);

/* From funiter.c */

XFUNCTION(perform_loop);
XFUNCTION(perform_iter);
XFUNCTION(fun_ilev);
XFUNCTION(fun_itext);
XFUNCTION(fun_itext2);
XFUNCTION(fun_ibreak);
XFUNCTION(fun_inum);
XFUNCTION(fun_fold);
XFUNCTION(handle_filter);
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
XFUNCTION(fun_lreplace);
XFUNCTION(fun_insert);
XFUNCTION(fun_remove);
XFUNCTION(fun_member);
XFUNCTION(fun_revwords);
XFUNCTION(fun_splice);
XFUNCTION(handle_sort);
XFUNCTION(fun_sortby);
XFUNCTION(handle_sets);
XFUNCTION(fun_columns);
XFUNCTION(process_tables);
XFUNCTION(fun_table);
XFUNCTION(fun_align);
XFUNCTION(fun_lalign);
XFUNCTION(fun_elements);
XFUNCTION(fun_exclude);
XFUNCTION(fun_grab);
XFUNCTION(fun_graball);
XFUNCTION(fun_shuffle);
XFUNCTION(fun_ledit);
XFUNCTION(fun_itemize);
XFUNCTION(fun_choose);
XFUNCTION(fun_group);
XFUNCTION(fun_tokens);

/* From funmath.c */

XFUNCTION(fun_pi);
XFUNCTION(fun_e);
XFUNCTION(fun_sign);
XFUNCTION(fun_abs);
XFUNCTION(fun_floor);
XFUNCTION(fun_ceil);
XFUNCTION(fun_round);
XFUNCTION(fun_trunc);
XFUNCTION(fun_inc);
XFUNCTION(fun_dec);
XFUNCTION(fun_sqrt);
XFUNCTION(fun_exp);
XFUNCTION(fun_ln);
XFUNCTION(fun_baseconv);
XFUNCTION(handle_trig);
XFUNCTION(fun_gt);
XFUNCTION(fun_gte);
XFUNCTION(fun_lt);
XFUNCTION(fun_lte);
XFUNCTION(fun_eq);
XFUNCTION(fun_neq);
XFUNCTION(fun_ncomp);
XFUNCTION(fun_sub);
XFUNCTION(fun_div);
XFUNCTION(fun_floordiv);
XFUNCTION(fun_fdiv);
XFUNCTION(fun_modulo);
XFUNCTION(fun_remainder);
XFUNCTION(fun_power);
XFUNCTION(fun_log);
XFUNCTION(fun_shl);
XFUNCTION(fun_shr);
XFUNCTION(fun_band);
XFUNCTION(fun_bor);
XFUNCTION(fun_bnand);
XFUNCTION(fun_add);
XFUNCTION(fun_mul);
XFUNCTION(fun_max);
XFUNCTION(fun_min);
XFUNCTION(fun_bound);
XFUNCTION(fun_dist2d);
XFUNCTION(fun_dist3d);
XFUNCTION(fun_ladd);
XFUNCTION(fun_lmax);
XFUNCTION(fun_lmin);
XFUNCTION(handle_vector);
XFUNCTION(handle_vectors);
XFUNCTION(fun_not);
XFUNCTION(fun_notbool);
XFUNCTION(fun_t);
XFUNCTION(handle_logic);
XFUNCTION(handle_listbool);

/* From funmisc.c */

XFUNCTION(fun_switchall);
XFUNCTION(fun_switch);
XFUNCTION(fun_case);
XFUNCTION(handle_ifelse);
XFUNCTION(fun_rand);
XFUNCTION(fun_die);
XFUNCTION(fun_lrand);
XFUNCTION(fun_lnum);
XFUNCTION(fun_time);
XFUNCTION(fun_secs);
XFUNCTION(fun_convsecs);
XFUNCTION(fun_convtime);
XFUNCTION(fun_timefmt);
XFUNCTION(fun_etimefmt);
XFUNCTION(fun_starttime);
XFUNCTION(fun_restarts);
XFUNCTION(fun_restarttime);
XFUNCTION(fun_version);
XFUNCTION(fun_mudname);
XFUNCTION(fun_hasmodule);
XFUNCTION(fun_connrecord);
XFUNCTION(fun_fcount);
XFUNCTION(fun_fdepth);
XFUNCTION(fun_ccount);
XFUNCTION(fun_cdepth);
XFUNCTION(fun_benchmark);
XFUNCTION(fun_s);
XFUNCTION(fun_subeval);
XFUNCTION(fun_link);
XFUNCTION(fun_tel);
XFUNCTION(fun_wipe);
XFUNCTION(fun_pemit);
XFUNCTION(fun_remit);
XFUNCTION(fun_oemit);
XFUNCTION(fun_force);
XFUNCTION(fun_trigger);
XFUNCTION(fun_wait);
XFUNCTION(fun_command);
XFUNCTION(fun_create);
XFUNCTION(fun_set);
XFUNCTION(fun_ps);
XFUNCTION(fun_nofx);

/* From funobj.c */

XFUNCTION(fun_objid);
XFUNCTION(fun_con);
XFUNCTION(fun_exit);
XFUNCTION(fun_next);
XFUNCTION(handle_loc);
XFUNCTION(fun_rloc);
XFUNCTION(fun_room);
XFUNCTION(fun_owner);
XFUNCTION(fun_controls);
XFUNCTION(fun_sees);
XFUNCTION(fun_nearby);
XFUNCTION(handle_name);
XFUNCTION(handle_pronoun);
XFUNCTION(fun_lock);
XFUNCTION(fun_elock);
XFUNCTION(fun_elockstr);
XFUNCTION(fun_xcon);
XFUNCTION(fun_lcon);
XFUNCTION(fun_lexits);
XFUNCTION(fun_entrances);
XFUNCTION(fun_home);
XFUNCTION(fun_money);
XFUNCTION(fun_findable);
XFUNCTION(fun_visible);
XFUNCTION(fun_writable);
XFUNCTION(fun_flags);
XFUNCTION(handle_flaglists);
XFUNCTION(fun_hasflag);
XFUNCTION(fun_haspower);
XFUNCTION(fun_hasflags);
XFUNCTION(handle_timestamp);
XFUNCTION(fun_parent);
XFUNCTION(fun_lparent);
XFUNCTION(fun_children);
XFUNCTION(fun_zone);
XFUNCTION(scan_zone);
XFUNCTION(fun_zfun);
XFUNCTION(fun_hasattr);
XFUNCTION(fun_v);
XFUNCTION(perform_get);
XFUNCTION(fun_eval);
XFUNCTION(do_ufun);
XFUNCTION(fun_objcall);
XFUNCTION(fun_localize);
XFUNCTION(fun_private);
XFUNCTION(fun_default);
XFUNCTION(fun_edefault);
XFUNCTION(fun_udefault);
XFUNCTION(fun_objeval);
XFUNCTION(fun_num);
XFUNCTION(fun_pmatch);
XFUNCTION(fun_pfind);
XFUNCTION(fun_locate);
XFUNCTION(handle_lattr);
XFUNCTION(fun_search);
XFUNCTION(fun_stats);
XFUNCTION(fun_objmem);
XFUNCTION(fun_playmem);
XFUNCTION(fun_type);
XFUNCTION(fun_hastype);
XFUNCTION(fun_lastcreate);
XFUNCTION(fun_speak);
XFUNCTION(handle_okpres);

/* From funstring.c */

XFUNCTION(fun_isword);
XFUNCTION(fun_isalnum);
XFUNCTION(fun_isnum);
XFUNCTION(fun_isdbref);
XFUNCTION(fun_isobjid);
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
XFUNCTION(fun_chomp);
XFUNCTION(fun_comp);
XFUNCTION(fun_streq);
XFUNCTION(fun_strmatch);
XFUNCTION(fun_edit);
XFUNCTION(fun_merge);
XFUNCTION(fun_secure);
XFUNCTION(fun_escape);
XFUNCTION(fun_esc);
XFUNCTION(fun_stripchars);
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
XFUNCTION(fun_diffpos);
XFUNCTION(fun_wordpos);
XFUNCTION(fun_ansipos);
XFUNCTION(fun_repeat);
XFUNCTION(perform_border);
XFUNCTION(fun_cat);
XFUNCTION(fun_strcat);
XFUNCTION(fun_join);
XFUNCTION(fun_strlen);
XFUNCTION(fun_delete);
XFUNCTION(fun_lit);
XFUNCTION(fun_art);
XFUNCTION(fun_alphamax);
XFUNCTION(fun_alphamin);
XFUNCTION(fun_valid);
XFUNCTION(fun_beep);

/* From funvars.c */

XFUNCTION(fun_setq);
XFUNCTION(fun_setr);
XFUNCTION(fun_r);
XFUNCTION(fun_wildmatch);
XFUNCTION(fun_qvars);
XFUNCTION(fun_qsub);
XFUNCTION(fun_lregs);
XFUNCTION(handle_ucall);
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
XFUNCTION(fun_read);
XFUNCTION(fun_delimit);
XFUNCTION(fun_z);
XFUNCTION(fun_modify);
XFUNCTION(fun_unload);
XFUNCTION(fun_write);
XFUNCTION(fun_destruct);
XFUNCTION(fun_unstructure);
XFUNCTION(fun_lstructures);
XFUNCTION(fun_linstances);
XFUNCTION(fun_empty);
XFUNCTION(fun_items);
XFUNCTION(fun_push);
XFUNCTION(fun_dup);
XFUNCTION(fun_swap);
XFUNCTION(handle_pop);
XFUNCTION(fun_popn);
XFUNCTION(fun_lstack);
XFUNCTION(perform_regedit);
XFUNCTION(fun_wildparse);
XFUNCTION(perform_regparse);
XFUNCTION(perform_regrab);
XFUNCTION(perform_regmatch);
XFUNCTION(fun_until);
XFUNCTION(perform_grep);
XFUNCTION(fun_gridmake);
XFUNCTION(fun_gridsize);
XFUNCTION(fun_gridset);
XFUNCTION(fun_grid);

/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * flist: List of existing functions in alphabetical order.
 */

FUN flist[] = {
{"@@",		fun_null,	1,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"ABS",		fun_abs,	1,  0,		CA_PUBLIC,	NULL},
{"ACOS",	handle_trig,	1,  TRIG_ARC|TRIG_CO,
						CA_PUBLIC,	NULL},
{"ACOSD",	handle_trig,	1,  TRIG_ARC|TRIG_CO|TRIG_DEG,
						CA_PUBLIC,	NULL},
{"ADD",		fun_add,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"AFTER",	fun_after,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"ALIGN",	fun_align,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"ALPHAMAX",	fun_alphamax,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"ALPHAMIN",	fun_alphamin,   0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"AND",		handle_logic,	0,  FN_VARARGS|LOGIC_AND,
						CA_PUBLIC,	NULL},
{"ANDBOOL",	handle_logic,	0,  FN_VARARGS|LOGIC_AND|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"ANDFLAGS",	handle_flaglists, 2, 0,		CA_PUBLIC,	NULL},
{"ANSI",        fun_ansi,       2,  0,          CA_PUBLIC,	NULL},
{"ANSIPOS",     fun_ansipos,    0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"APOSS",	handle_pronoun,	1,  PRONOUN_APOSS, CA_PUBLIC,	NULL},
{"ART",		fun_art,	1,  0,		CA_PUBLIC,	NULL},
{"ASIN",	handle_trig,	1,  TRIG_ARC,	CA_PUBLIC,	NULL},
{"ASIND",	handle_trig,	1,  TRIG_ARC|TRIG_DEG,
						CA_PUBLIC,	NULL},
{"ATAN",	handle_trig,	1,  TRIG_ARC|TRIG_TAN,
						CA_PUBLIC,	NULL},
{"ATAND",	handle_trig,	1,  TRIG_ARC|TRIG_TAN|TRIG_DEG,
						CA_PUBLIC,	NULL},
{"BAND",	fun_band,	2,  0,		CA_PUBLIC,	NULL},
{"BASECONV",	fun_baseconv,	3,  0,		CA_PUBLIC,	NULL},
{"BEEP",        fun_beep,       0,  0,          CA_WIZARD,	NULL},
{"BEFORE",	fun_before,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"BENCHMARK",   fun_benchmark,  2,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"BNAND",	fun_bnand,	2,  0,		CA_PUBLIC,	NULL},
{"BOUND",	fun_bound,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"BOR",		fun_bor,	2,  0,		CA_PUBLIC,	NULL},
{"BORDER",	perform_border,	0,  FN_VARARGS|JUST_LEFT,
						CA_PUBLIC,	NULL},
{"CANDBOOL",	handle_logic,	0,  FN_VARARGS|FN_NO_EVAL|LOGIC_AND|LOGIC_BOOL,
     						CA_PUBLIC,	NULL},
{"CAND",	handle_logic,	0,  FN_VARARGS|FN_NO_EVAL|LOGIC_AND,
     						CA_PUBLIC,	NULL},
{"CAPSTR",	fun_capstr,	-1, 0,		CA_PUBLIC,	NULL},
{"CASE",	fun_case,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"CAT",		fun_cat,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"CBORDER",	perform_border,	0,  FN_VARARGS|JUST_CENTER,
						CA_PUBLIC,	NULL},
{"CCOUNT",	fun_ccount,	0,  0,		CA_PUBLIC,	NULL},
{"CDEPTH",	fun_cdepth,	0,  0,		CA_PUBLIC,	NULL},
{"CEIL",	fun_ceil,	1,  0,		CA_PUBLIC,	NULL},
{"CENTER",	fun_center,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"CHILDREN",    fun_children,   0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"CHOMP",	fun_chomp,	1,  0,		CA_PUBLIC,	NULL},
{"CHOOSE",	fun_choose,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"CLEARVARS",	fun_clearvars,	0,  FN_VARFX,	CA_PUBLIC,	NULL},
{"COLUMNS",	fun_columns,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"COMMAND",	fun_command,	0,  FN_VARARGS|FN_DBFX, CA_PUBLIC,	NULL},
{"COMP",	fun_comp,	2,  0,		CA_PUBLIC,	NULL},
{"CON",		fun_con,	1,  0,		CA_PUBLIC,	NULL},
{"CONFIG",	fun_config,	1,  0,		CA_PUBLIC,	NULL},
{"CONN",	handle_conninfo, 1, 0,		CA_PUBLIC,	NULL},
{"CONNRECORD",	fun_connrecord,	0,  0,		CA_PUBLIC,	NULL},
{"CONSTRUCT",	fun_construct,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"CONTROLS", 	fun_controls,	2,  0,		CA_PUBLIC,	NULL},
{"CONVSECS",    fun_convsecs,   1,  0,		CA_PUBLIC,	NULL},
{"CONVTIME",    fun_convtime,   1,  0,		CA_PUBLIC,	NULL},
{"COR",		handle_logic,	0,  FN_VARARGS|FN_NO_EVAL|LOGIC_OR,
     						CA_PUBLIC,	NULL},
{"CORBOOL",	handle_logic,	0,  FN_VARARGS|FN_NO_EVAL|LOGIC_OR|LOGIC_BOOL,
     						CA_PUBLIC,	NULL},
{"COS",		handle_trig,	1,  TRIG_CO,	CA_PUBLIC,	NULL},
{"COSD",	handle_trig,	1,  TRIG_CO|TRIG_DEG,
						CA_PUBLIC,	NULL},
{"CREATE",      fun_create,     0,  FN_VARARGS|FN_DBFX, CA_PUBLIC,	NULL},
{"CREATION",	handle_timestamp, 1, TIMESTAMP_CRE, CA_PUBLIC,	NULL},
{"CTABLES",	process_tables,	0,  FN_VARARGS|JUST_CENTER,
						CA_PUBLIC,	NULL},
{"DEC",         fun_dec,        1,  0,          CA_PUBLIC,	NULL},
{"DECRYPT",	fun_decrypt,	2,  0,		CA_PUBLIC,	NULL},
{"DEFAULT",	fun_default,	2,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"DELETE",	fun_delete,	3,  0,		CA_PUBLIC,	NULL},
{"DELIMIT",	fun_delimit,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"DESTRUCT",	fun_destruct,	1,  0,		CA_PUBLIC|FN_VARFX,	NULL},
{"DIE",		fun_die,	2,  0,		CA_PUBLIC,	NULL},
{"DIFFPOS",	fun_diffpos,	2,  0,		CA_PUBLIC,	NULL},
{"DIST2D",	fun_dist2d,	4,  0,		CA_PUBLIC,	NULL},
{"DIST3D",	fun_dist3d,	6,  0,		CA_PUBLIC,	NULL},
{"DIV",		fun_div,	2,  0,		CA_PUBLIC,	NULL},
{"DOING",	fun_doing,	1,  0,		CA_PUBLIC,	NULL},
{"DUP",		fun_dup,	0,  FN_VARARGS|FN_STACKFX,	CA_PUBLIC,	NULL},
{"E",		fun_e,		0,  0,		CA_PUBLIC,	NULL},
{"EDEFAULT",	fun_edefault,	2,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"EDIT",	fun_edit,	3,  0,		CA_PUBLIC,	NULL},
{"ELEMENTS",	fun_elements,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"ELOCK",	fun_elock,	2,  0,		CA_PUBLIC,	NULL},
{"ELOCKSTR",	fun_elockstr,	3,  0,		CA_PUBLIC,	NULL},
{"EMPTY",	fun_empty,	0,  FN_VARARGS|FN_STACKFX, CA_PUBLIC,	NULL},
{"ENCRYPT",	fun_encrypt,	2,  0,		CA_PUBLIC,	NULL},
{"ENTRANCES",	fun_entrances,	0,  FN_VARARGS,	CA_NO_GUEST,	NULL},
{"EQ",		fun_eq,		2,  0,		CA_PUBLIC,	NULL},
{"ESC",		fun_esc,	-1, 0,		CA_PUBLIC,	NULL},
{"ESCAPE",	fun_escape,	-1, 0,		CA_PUBLIC,	NULL},
{"ETIMEFMT",	fun_etimefmt,	2,  0,		CA_PUBLIC,	NULL},
{"EXCLUDE",	fun_exclude,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"EXIT",	fun_exit,	1,  0,		CA_PUBLIC,	NULL},
{"EXP",		fun_exp,	1,  0,		CA_PUBLIC,	NULL},
{"EXTRACT",	fun_extract,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"EVAL",        fun_eval,       0,  FN_VARARGS|GET_EVAL|GET_XARGS,
						CA_PUBLIC,	NULL},
{"SUBEVAL",  	fun_subeval,	1,  0,		CA_PUBLIC,	NULL},
{"FCOUNT",	fun_fcount,	0,  0,		CA_PUBLIC,	NULL},
{"FDEPTH",	fun_fdepth,	0,  0,		CA_PUBLIC,	NULL},
{"FDIV",	fun_fdiv,	2,  0,		CA_PUBLIC,	NULL},
{"FILTER",	handle_filter,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"FILTERBOOL",	handle_filter,	0,  FN_VARARGS|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"FINDABLE",	fun_findable,	2,  0,		CA_PUBLIC,	NULL},
{"FIRST",	fun_first,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"FLAGS",	fun_flags,	1,  0,		CA_PUBLIC,	NULL},
{"FLOOR",	fun_floor,	1,  0,		CA_PUBLIC,	NULL},
{"FLOORDIV",	fun_floordiv,	2,  0,		CA_PUBLIC,	NULL},
{"FOLD",	fun_fold,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"FORCE",	fun_force,	2,  FN_QFX,		CA_PUBLIC,	NULL},
{"FOREACH",	fun_foreach,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"FULLNAME",	handle_name,	1,  NAMEFN_FULLNAME, CA_PUBLIC,	NULL},
{"GET",		perform_get,	1,  0,		CA_PUBLIC,	NULL},
{"GET_EVAL",	perform_get,	1,  GET_EVAL,	CA_PUBLIC,	NULL},
{"GRAB",	fun_grab,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GRABALL",	fun_graball,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GREP",	perform_grep,	0,  FN_VARARGS|GREP_EXACT,	CA_PUBLIC,	NULL},
{"GREPI",	perform_grep,	0,  FN_VARARGS|GREP_EXACT|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"GRID",	fun_grid,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GRIDMAKE",	fun_gridmake,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GRIDSET",	fun_gridset,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GRIDSIZE",	fun_gridsize,	0,  0,		CA_PUBLIC,	NULL},
{"GROUP",	fun_group,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"GT",		fun_gt,		2,  0,		CA_PUBLIC,	NULL},
{"GTE",		fun_gte,	2,  0,		CA_PUBLIC,	NULL},
{"HASATTR",	fun_hasattr,	2,  0,		CA_PUBLIC,	NULL},
{"HASATTRP",	fun_hasattr,	2,  CHECK_PARENTS,
						CA_PUBLIC,	NULL},
{"HASFLAG",	fun_hasflag,	2,  0,		CA_PUBLIC,	NULL},
{"HASFLAGS",	fun_hasflags,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"HASMODULE",	fun_hasmodule,	1,  0,		CA_PUBLIC,	NULL},
{"HASPOWER",    fun_haspower,   2,  0,          CA_PUBLIC,	NULL},
{"HASTYPE",	fun_hastype,	2,  0,		CA_PUBLIC,	NULL},
{"HEARS",	handle_okpres,	2,  PRESFN_HEARS,
						CA_PUBLIC,	NULL},
{"HELPTEXT",	fun_helptext,	2,  0,		CA_PUBLIC,	NULL},
{"HOME",	fun_home,	1,  0,		CA_PUBLIC,	NULL},
#ifdef PUEBLO_SUPPORT
{"HTML_ESCAPE",	fun_html_escape,-1, 0,		CA_PUBLIC,	NULL},
{"HTML_UNESCAPE",fun_html_unescape,-1,0,	CA_PUBLIC,	NULL},
#endif /* PUEBLO_SUPPORT */
{"IBREAK",	fun_ibreak,	1,  0,		CA_PUBLIC,	NULL},
{"IDLE",	handle_conninfo, 1, CONNINFO_IDLE, CA_PUBLIC,	NULL},
{"IFELSE",      handle_ifelse,   0, IFELSE_BOOL|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"IFTRUE",      handle_ifelse,   0, IFELSE_TOKEN|IFELSE_BOOL|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"IFFALSE",     handle_ifelse,   0, IFELSE_FALSE|IFELSE_TOKEN|IFELSE_BOOL|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"IFZERO",     handle_ifelse,   0,  IFELSE_FALSE|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"ILEV",	fun_ilev,	0,  0,		CA_PUBLIC,	NULL},
{"INC",         fun_inc,        1,  0,          CA_PUBLIC,	NULL},
{"INDEX",	fun_index,	4,  0,		CA_PUBLIC,	NULL},
{"INSERT",	fun_insert,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"INUM",	fun_inum,	1,  0,		CA_PUBLIC,	NULL},
{"INZONE",	scan_zone,	1,  TYPE_ROOM,	CA_PUBLIC,	NULL},
{"ISALNUM",	fun_isalnum,	1,  0,		CA_PUBLIC,	NULL},
{"ISDBREF",	fun_isdbref,	1,  0,		CA_PUBLIC,	NULL},
{"ISNUM",	fun_isnum,	1,  0,		CA_PUBLIC,	NULL},
{"ISOBJID",	fun_isobjid,	1,  0,		CA_PUBLIC,	NULL},
{"ISWORD",	fun_isword,	1,  0,		CA_PUBLIC,	NULL},
{"ISORT",	handle_sort,	0,  FN_VARARGS|SORT_POS,	CA_PUBLIC,	NULL},
{"ITEMIZE",	fun_itemize,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"ITEMS",	fun_items,	0,  FN_VARARGS|FN_STACKFX,	CA_PUBLIC,	NULL},
{"ITER",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_NONE|FILT_COND_NONE,
						CA_PUBLIC,	NULL},
{"ITER2",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_NONE|FILT_COND_NONE|LOOP_TWOLISTS,
						CA_PUBLIC,	NULL},
{"ISFALSE",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_NONE|FILT_COND_FALSE,
						CA_PUBLIC,	NULL},
{"ISTRUE",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_NONE|FILT_COND_TRUE,
						CA_PUBLIC,	NULL},
{"ITEXT",	fun_itext,	1,  0,		CA_PUBLIC,	NULL},
{"ITEXT2",	fun_itext2,	1,  0,		CA_PUBLIC,	NULL},
{"JOIN",	fun_join,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"KNOWS",	handle_okpres,	2,  PRESFN_KNOWS,
						CA_PUBLIC,	NULL},
{"LADD",	fun_ladd,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LALIGN",	fun_lalign,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LAND",	handle_logic,	0,  FN_VARARGS|LOGIC_LIST|LOGIC_AND,
						CA_PUBLIC,	NULL},
{"LANDBOOL",	handle_logic,	0,  FN_VARARGS|LOGIC_LIST|LOGIC_AND|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"LAST",	fun_last,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LASTACCESS",	handle_timestamp, 1, TIMESTAMP_ACC, CA_PUBLIC,	NULL},
{"LASTCREATE",	fun_lastcreate,	2,  0,		CA_PUBLIC,	NULL},
{"LASTMOD",	handle_timestamp, 1, TIMESTAMP_MOD, CA_PUBLIC,	NULL},
{"LATTR",	handle_lattr,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LCON",	fun_lcon,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LCSTR",	fun_lcstr,	-1, 0,		CA_PUBLIC,	NULL},
{"LDELETE",	fun_ldelete,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LDIFF",	handle_sets,	0,  FN_VARARGS|SET_TYPE|SET_DIFF,
						CA_PUBLIC,	NULL},
{"LEDIT",	fun_ledit,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"LEFT",	fun_left,	2,  0,		CA_PUBLIC,	NULL},
{"LET",		fun_let,	0,  FN_VARARGS|FN_NO_EVAL|FN_VARFX,
						CA_PUBLIC,	NULL},
{"LEXITS",	fun_lexits,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LFALSE",	handle_listbool, 0, FN_VARARGS|IFELSE_BOOL|IFELSE_FALSE,
 						CA_PUBLIC,	NULL},
{"LIST",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|FN_OUTFX|BOOL_COND_NONE|FILT_COND_NONE|LOOP_NOTIFY,
						CA_PUBLIC}, 
{"LIST2",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|FN_OUTFX|BOOL_COND_NONE|FILT_COND_NONE|LOOP_NOTIFY|LOOP_TWOLISTS,
						CA_PUBLIC}, 
{"LIT",		fun_lit,	-1, FN_NO_EVAL,	CA_PUBLIC,	NULL},
{"LINK",	fun_link,	2,  FN_DBFX,	CA_PUBLIC,	NULL},
{"LINSTANCES",	fun_linstances,	0,  FN_VARFX,		CA_PUBLIC,	NULL},
{"LINTER",	handle_sets,	0,  FN_VARARGS|SET_TYPE|SET_INTERSECT,
						CA_PUBLIC,	NULL},
{"LJUST",	fun_ljust,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LMAX",	fun_lmax,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LMIN",	fun_lmin,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LN",		fun_ln,		1,  0,		CA_PUBLIC,	NULL},
{"LNUM",	fun_lnum,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LOAD",	fun_load,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"LOC",		handle_loc,	1,  0,		CA_PUBLIC,	NULL},
{"LOCATE",	fun_locate,	3,  0,		CA_PUBLIC,	NULL},
{"LOCALIZE",    fun_localize,   1,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"LOCK",	fun_lock,	1,  0,		CA_PUBLIC,	NULL},
{"LOG",		fun_log,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LPARENT",	fun_lparent,	0,  FN_VARARGS,	CA_PUBLIC}, 
{"LOOP",	perform_loop,	0,  FN_VARARGS|FN_NO_EVAL|FN_OUTFX|LOOP_NOTIFY,
						CA_PUBLIC,	NULL},
{"LOR",		handle_logic,	0,  FN_VARARGS|LOGIC_LIST|LOGIC_OR,
						CA_PUBLIC,	NULL},
{"LORBOOL",	handle_logic,	0,  FN_VARARGS|LOGIC_LIST|LOGIC_OR|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"LPOS",	fun_lpos,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LRAND",	fun_lrand,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"LREGS",	fun_lregs,	0,  0,		CA_PUBLIC,	NULL},
{"LREPLACE",	fun_lreplace,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"LSTACK",	fun_lstack,	0,  FN_VARARGS|FN_STACKFX, CA_PUBLIC,	NULL},
{"LSTRUCTURES",	fun_lstructures, 0, FN_VARFX,	CA_PUBLIC,	NULL},
{"LT",		fun_lt,		2,  0,		CA_PUBLIC,	NULL},
{"LTE",		fun_lte,	2,  0,		CA_PUBLIC,	NULL},
{"LTRUE",	handle_listbool, 0, FN_VARARGS|IFELSE_BOOL,
 						CA_PUBLIC,	NULL},
{"LUNION",	handle_sets,	0,  FN_VARARGS|SET_TYPE|SET_UNION,
						CA_PUBLIC,	NULL},
{"LVARS",	fun_lvars,	0,  FN_VARFX,	CA_PUBLIC,	NULL},
{"LWHO",	fun_lwho,	0,  0,		CA_PUBLIC,	NULL},
{"MAP",		fun_map,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MATCH",	fun_match,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MATCHALL",	fun_matchall,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MAX",		fun_max,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MEMBER",	fun_member,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MERGE",	fun_merge,	3,  0,		CA_PUBLIC,	NULL},
{"MID",		fun_mid,	3,  0,		CA_PUBLIC,	NULL},
{"MIN",		fun_min,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MIX",		fun_mix,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MODULO",	fun_modulo,	2,  0,		CA_PUBLIC,	NULL},
{"MODIFY",	fun_modify,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"MONEY",	fun_money,	1,  0,		CA_PUBLIC,	NULL},
{"MOVES",	handle_okpres,	2,  PRESFN_MOVES,
						CA_PUBLIC,	NULL},
{"MUDNAME",	fun_mudname,	0,  0,		CA_PUBLIC,	NULL},
{"MUL",		fun_mul,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"MUNGE",	fun_munge,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"NAME",	handle_name,	1,  0,		CA_PUBLIC,	NULL},
{"NATTR",	handle_lattr,	1,  LATTR_COUNT,
						CA_PUBLIC,	NULL},
{"NCOMP",	fun_ncomp,	2,  0,		CA_PUBLIC,	NULL},
{"NEARBY",	fun_nearby,	2,  0,		CA_PUBLIC,	NULL},
{"NEQ",		fun_neq,	2,  0,		CA_PUBLIC,	NULL},
{"NESCAPE",	fun_escape,	-1, FN_NO_EVAL,	CA_PUBLIC,	NULL},
{"NEXT",	fun_next,	1,  0,		CA_PUBLIC,	NULL},
{"NOFX",	fun_nofx,	2,  FN_NO_EVAL,	CA_PUBLIC,	NULL},
{"NONZERO",     handle_ifelse,  0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"NOT",		fun_not,	1,  0,		CA_PUBLIC,	NULL},
{"NOTBOOL",	fun_notbool,	1,  0,		CA_PUBLIC,	NULL},
{"NSECURE",	fun_secure,	-1, FN_NO_EVAL,	CA_PUBLIC,	NULL},
{"NULL",	fun_null,	1,  0,		CA_PUBLIC,	NULL},
{"NUM",		fun_num,	1,  0,		CA_PUBLIC,	NULL},
{"OBJ",		handle_pronoun,	1,  PRONOUN_OBJ, CA_PUBLIC,	NULL},
{"OBJCALL",     fun_objcall,    0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"OBJEVAL",     fun_objeval,    2,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"OBJID",	fun_objid,	1,  0,		CA_PUBLIC,	NULL},
{"OBJMEM",	fun_objmem,	1,  0,		CA_PUBLIC,	NULL},
{"OEMIT",	fun_oemit,	2,  FN_OUTFX,	CA_PUBLIC,	NULL},
{"OR",		handle_logic,	0,  FN_VARARGS|LOGIC_OR,
						CA_PUBLIC,	NULL},
{"ORBOOL",	handle_logic,	0,  FN_VARARGS|LOGIC_OR|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"ORFLAGS",	handle_flaglists, 2, LOGIC_OR,	CA_PUBLIC,	NULL},
{"OWNER",	fun_owner,	1,  0,		CA_PUBLIC,	NULL},
{"PARENT",	fun_parent,	1,  0,		CA_PUBLIC,	NULL},
{"PARSE",	perform_loop,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"PEEK",	handle_pop,	0,  FN_VARARGS|FN_STACKFX|POP_PEEK,
						CA_PUBLIC,	NULL},
{"PEMIT",	fun_pemit,	2,  FN_OUTFX,	CA_PUBLIC,	NULL},
{"PFIND",	fun_pfind,	1,  0,		CA_PUBLIC,	NULL},
{"PI",		fun_pi,		0,  0,		CA_PUBLIC,	NULL},
{"PLAYMEM",	fun_playmem,	1,  0,		CA_PUBLIC,	NULL},
{"PMATCH",	fun_pmatch,	1,  0,		CA_PUBLIC,	NULL},
{"POP",		handle_pop,	0,  FN_VARARGS|FN_STACKFX, CA_PUBLIC,	NULL},
{"POPN",	fun_popn,	0,  FN_VARARGS|FN_STACKFX, CA_PUBLIC,	NULL},
{"PORTS",	fun_ports,	0,  FN_VARARGS,	CA_WIZARD,	NULL},
{"POS",		fun_pos,	2,  0,		CA_PUBLIC,	NULL},
{"POSS",	handle_pronoun,	1,  PRONOUN_POSS, CA_PUBLIC,	NULL},
{"POWER",	fun_power,	2,  0,		CA_PUBLIC,	NULL},
{"PRIVATE",     fun_private,    1,  FN_NO_EVAL, CA_PUBLIC,	NULL},
{"PROGRAMMER",	fun_programmer,	1,  0,		CA_PUBLIC,	NULL},
{"PS",		fun_ps,		1,  0,		CA_PUBLIC,	NULL},
{"PUSH",	fun_push,	0,  FN_VARARGS|FN_STACKFX, CA_PUBLIC,	NULL},
{"QSUB",	fun_qsub,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"QVARS",	fun_qvars,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"R",		fun_r,		1,  0,		CA_PUBLIC,	NULL},
{"RAND",	fun_rand,	1,  0,		CA_PUBLIC,	NULL},
{"RBORDER",	perform_border,	0,  FN_VARARGS|JUST_RIGHT,
						CA_PUBLIC,	NULL},
{"READ",	fun_read,	3,  FN_VARFX,		CA_PUBLIC,	NULL},
{"REGEDIT",	perform_regedit, 3, 0,		CA_PUBLIC,	NULL},
{"REGEDITALL",	perform_regedit, 3, REG_MATCH_ALL,
						CA_PUBLIC,	NULL},
{"REGEDITALLI",	perform_regedit, 3, REG_MATCH_ALL|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGEDITI",	perform_regedit, 3, REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGRAB",	perform_regrab,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"REGRABALL",	perform_regrab,	0,  FN_VARARGS|REG_MATCH_ALL,
						CA_PUBLIC,	NULL},
{"REGRABALLI",	perform_regrab, 0,  FN_VARARGS|REG_MATCH_ALL|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGRABI",	perform_regrab,	0,  FN_VARARGS|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGREP",	perform_grep,	0,  FN_VARARGS|GREP_REGEXP,
						CA_PUBLIC,	NULL},
{"REGREPI",	perform_grep,	0,  FN_VARARGS|GREP_REGEXP|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGMATCH",	perform_regmatch, 0, FN_VARARGS,
						CA_PUBLIC,	NULL},
{"REGMATCHI",	perform_regmatch, 0, FN_VARARGS|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REGPARSE",	perform_regparse, 3, FN_VARFX,	CA_PUBLIC,	NULL},
{"REGPARSEI",	perform_regparse, 3, FN_VARFX|REG_CASELESS,
						CA_PUBLIC,	NULL},
{"REMAINDER",	fun_remainder,	2,  0,		CA_PUBLIC,	NULL},
{"REMIT",	fun_remit,	2,  FN_OUTFX,	CA_PUBLIC,	NULL},
{"REMOVE",	fun_remove,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"REPEAT",	fun_repeat,	2,  0,		CA_PUBLIC,	NULL},
{"REPLACE",	fun_replace,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"REST",	fun_rest,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"RESTARTS",	fun_restarts,	0,  0,		CA_PUBLIC,	NULL},
{"RESTARTTIME",	fun_restarttime, 0, 0,		CA_PUBLIC,	NULL},
{"REVERSE",	fun_reverse,	-1, 0,		CA_PUBLIC,	NULL},
{"REVWORDS",	fun_revwords,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"RIGHT",	fun_right,	2,  0,		CA_PUBLIC,	NULL},
{"RJUST",	fun_rjust,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"RLOC",	fun_rloc,	2,  0,		CA_PUBLIC,	NULL},
{"ROOM",	fun_room,	1,  0,		CA_PUBLIC,	NULL},
{"ROUND",	fun_round,	2,  0,		CA_PUBLIC,	NULL},
{"RTABLES",	process_tables,	0,  FN_VARARGS|JUST_RIGHT,
						CA_PUBLIC,	NULL},
{"S",		fun_s,		-1, 0,		CA_PUBLIC,	NULL},
{"SANDBOX",	handle_ucall,	0,  FN_VARARGS|UCALL_SANDBOX,
						CA_PUBLIC,	NULL},
{"SCRAMBLE",	fun_scramble,	1,  0,		CA_PUBLIC,	NULL},
{"SEARCH",	fun_search,	-1, 0,		CA_PUBLIC,	NULL},
{"SECS",	fun_secs,	0,  0,		CA_PUBLIC,	NULL},
{"SECURE",	fun_secure,	-1, 0,		CA_PUBLIC,	NULL},
{"SEES",	fun_sees,	2,  0,		CA_PUBLIC,	NULL},
{"SESSION",	fun_session,	1,  0,		CA_PUBLIC,	NULL},
{"SET",		fun_set,	2,  0,		CA_PUBLIC,	NULL},
{"SETDIFF",	handle_sets,	0,  FN_VARARGS|SET_DIFF,
						CA_PUBLIC,	NULL},
{"SETINTER",	handle_sets,	0,  FN_VARARGS|SET_INTERSECT,
						CA_PUBLIC,	NULL},
{"SETQ",	fun_setq,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"SETR",	fun_setr,	2,  0,		CA_PUBLIC,	NULL},
{"SETX",	fun_setx,	2,  FN_VARFX,	CA_PUBLIC,	NULL},
{"SETUNION",	handle_sets,	0,  FN_VARARGS|SET_UNION,
						CA_PUBLIC,	NULL},
{"SHL",		fun_shl,	2,  0,		CA_PUBLIC,	NULL},
{"SHR",		fun_shr,	2,  0,		CA_PUBLIC,	NULL},
{"SHUFFLE",	fun_shuffle,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"SIGN",	fun_sign,	1,  0,		CA_PUBLIC,	NULL},
{"SIN",		handle_trig,	1,  0,		CA_PUBLIC,	NULL},
{"SIND",	handle_trig,	1,  TRIG_DEG,	CA_PUBLIC,	NULL},
{"SORT",	handle_sort,	0,  FN_VARARGS|SORT_ITEMS,	CA_PUBLIC,	NULL},
{"SORTBY",	fun_sortby,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"SPACE",	fun_space,	1,  0,		CA_PUBLIC,	NULL},
{"SPEAK",	fun_speak,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"SPLICE",	fun_splice,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"SQL",		fun_sql,	0,  FN_VARARGS, CA_SQL_OK,	NULL},
{"SQRT",	fun_sqrt,	1,  0,		CA_PUBLIC,	NULL},
{"SQUISH",	fun_squish,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"STARTTIME",	fun_starttime,	0,  0,		CA_PUBLIC,	NULL},
{"STATS",	fun_stats,	1,  0,		CA_PUBLIC,	NULL},
{"STEP",	fun_step,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"STORE",	fun_store,	2,  FN_VARFX,	CA_PUBLIC,	NULL},
{"STRCAT",	fun_strcat,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"STREQ",	fun_streq,	2,  0,		CA_PUBLIC,	NULL},
{"STRIPANSI",	fun_stripansi,	1,  0,		CA_PUBLIC,	NULL},
{"STRIPCHARS",	fun_stripchars,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"STRLEN",	fun_strlen,	-1, 0,		CA_PUBLIC,	NULL},
{"STRMATCH",	fun_strmatch,	2,  0,		CA_PUBLIC,	NULL},
{"STRTRUNC",    fun_left,	2,  0,          CA_PUBLIC,	NULL},
{"STRUCTURE",	fun_structure,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"SUB",		fun_sub,	2,  0,		CA_PUBLIC,	NULL},
{"SUBJ",	handle_pronoun,	1,  PRONOUN_SUBJ, CA_PUBLIC,	NULL},
{"SWAP",	fun_swap,	0,  FN_VARARGS|FN_STACKFX,	CA_PUBLIC,	NULL},
{"SWITCH",	fun_switch,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"SWITCHALL",	fun_switchall,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"T",		fun_t,		1,  0,		CA_PUBLIC,	NULL},
{"TABLE",	fun_table,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"TABLES",	process_tables,	0,  FN_VARARGS|JUST_LEFT,
						CA_PUBLIC,	NULL},
{"TAN",		handle_trig,	1,  TRIG_TAN,	CA_PUBLIC,	NULL},
{"TAND",	handle_trig,	1,  TRIG_TAN|TRIG_DEG,
						CA_PUBLIC,	NULL},
{"TEL",		fun_tel,	2,  0,		CA_PUBLIC,	NULL},
{"TIME",	fun_time,	0,  0,		CA_PUBLIC,	NULL},
{"TIMEFMT",	fun_timefmt,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"TOKENS",	fun_tokens,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"TOSS",	handle_pop,	0,  FN_VARARGS|FN_STACKFX|POP_TOSS,
						CA_PUBLIC,	NULL},
{"TRANSLATE",	fun_translate,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"TRIGGER",	fun_trigger,	0,  FN_VARARGS|FN_QFX, CA_PUBLIC,	NULL},
{"TRIM",	fun_trim,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"TRUNC",	fun_trunc,	1,  0,		CA_PUBLIC,	NULL},
{"TYPE",	fun_type,	1,  0,		CA_PUBLIC,	NULL},
{"U",		do_ufun,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"UCALL",	handle_ucall,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"UCSTR",	fun_ucstr,	-1, 0,		CA_PUBLIC,	NULL},
{"UDEFAULT",	fun_udefault,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"ULOCAL",	do_ufun,	0,  FN_VARARGS|U_LOCAL,
						CA_PUBLIC,	NULL},
{"UNLOAD",	fun_unload,	0,  FN_VARARGS|FN_VARFX,	CA_PUBLIC,	NULL},
{"UNMATCHALL",	fun_matchall,	0,  FN_VARARGS|IFELSE_FALSE,	CA_PUBLIC,	NULL},
{"UNSTRUCTURE",	fun_unstructure,1,  FN_VARFX,	CA_PUBLIC,	NULL},
{"UNTIL",	fun_until,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"UPRIVATE",	do_ufun,	0,  FN_VARARGS|U_PRIVATE,
						CA_PUBLIC,	NULL},
#ifdef PUEBLO_SUPPORT
{"URL_ESCAPE",	fun_url_escape,	-1, 0,		CA_PUBLIC,	NULL},
{"URL_UNESCAPE",fun_url_unescape,-1,0,		CA_PUBLIC,	NULL},
#endif /* PUEBLO_SUPPORT */
{"USETRUE",      handle_ifelse, 0, IFELSE_DEFAULT|IFELSE_BOOL|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"USEFALSE",     handle_ifelse, 0, IFELSE_FALSE|IFELSE_DEFAULT|IFELSE_BOOL|FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC,	NULL},
{"V",		fun_v,		1,  0,		CA_PUBLIC,	NULL},
{"VADD",	handle_vectors,	0,  FN_VARARGS|VEC_ADD,
						CA_PUBLIC,	NULL},
{"VAND",	handle_vectors,	0,  FN_VARARGS|VEC_AND,
						CA_PUBLIC,	NULL},
{"VALID",	fun_valid,	2,  FN_VARARGS, CA_PUBLIC,	NULL},
{"VDIM",	fun_words,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"VDOT",	handle_vectors,	0,  FN_VARARGS|VEC_DOT,
						CA_PUBLIC,	NULL},
{"VERSION",	fun_version,	0,  0,		CA_PUBLIC,	NULL},
{"VISIBLE",	fun_visible,	2,  0,		CA_PUBLIC,	NULL},
{"VMAG",	handle_vector,	0,  FN_VARARGS|VEC_MAG,
						CA_PUBLIC,	NULL},
{"VMUL",	handle_vectors,	0,  FN_VARARGS|VEC_MUL,
						CA_PUBLIC,	NULL},
{"VOR",		handle_vectors,	0,  FN_VARARGS|VEC_OR,
						CA_PUBLIC,	NULL},
{"VSUB",	handle_vectors,	0,  FN_VARARGS|VEC_SUB,
						CA_PUBLIC,	NULL},
{"VUNIT",	handle_vector,	0,  FN_VARARGS|VEC_UNIT,
						CA_PUBLIC,	NULL},
{"VXOR",	handle_vectors,	0,  FN_VARARGS|VEC_XOR,
						CA_PUBLIC,	NULL},
{"WAIT",	fun_wait,	2,  FN_QFX,	CA_PUBLIC,	NULL},
{"WHENFALSE",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_FALSE|FILT_COND_NONE,
						CA_PUBLIC,	NULL},
{"WHENTRUE",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_TRUE|FILT_COND_NONE,
						CA_PUBLIC,	NULL},
{"WHENFALSE2",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_FALSE|FILT_COND_NONE|LOOP_TWOLISTS,
						CA_PUBLIC,	NULL},
{"WHENTRUE2",	perform_iter,	0,  FN_VARARGS|FN_NO_EVAL|BOOL_COND_TRUE|FILT_COND_NONE|LOOP_TWOLISTS,
						CA_PUBLIC,	NULL},
{"WHERE",	handle_loc,	1,  LOCFN_WHERE, CA_PUBLIC,	NULL},
{"WHILE",	fun_while,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"WILDGREP",	perform_grep,	0,  FN_VARARGS|GREP_WILD,	CA_PUBLIC,	NULL},
{"WILDMATCH",	fun_wildmatch,	3,  0,		CA_PUBLIC,	NULL},
{"WILDPARSE",	fun_wildparse,	3,  FN_VARFX,	CA_PUBLIC,	NULL},
{"WIPE",	fun_wipe,	1,  FN_DBFX,	CA_PUBLIC,	NULL},
{"WORDPOS",     fun_wordpos,    0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"WORDS",	fun_words,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"WRITABLE",	fun_writable,	2,  0,		CA_PUBLIC,	NULL},
{"WRITE",	fun_write,	2,  FN_VARFX,	CA_PUBLIC,	NULL},
{"X",		fun_x,		1,  FN_VARFX,	CA_PUBLIC,	NULL},
{"XCON",	fun_xcon,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"XGET",	perform_get,	2,  GET_XARGS,	CA_PUBLIC,	NULL},
{"XOR",		handle_logic,	0,  FN_VARARGS|LOGIC_XOR,
						CA_PUBLIC,	NULL},
{"XORBOOL",	handle_logic,	0,  FN_VARARGS|LOGIC_XOR|LOGIC_BOOL,
						CA_PUBLIC,	NULL},
{"XVARS",	fun_xvars,	0,  FN_VARARGS|FN_VARFX, CA_PUBLIC,	NULL},
{"Z",		fun_z,		2,  FN_VARFX,	CA_PUBLIC,	NULL},
{"ZFUN",	fun_zfun,	0,  FN_VARARGS,	CA_PUBLIC,	NULL},
{"ZONE",        fun_zone,       1,  0,          CA_PUBLIC,	NULL},
{"ZWHO",	scan_zone,	1,  TYPE_PLAYER,
						CA_PUBLIC,	NULL},
{NULL,		NULL,		0,  0,		0,		NULL}};

/* *INDENT-ON* */

#endif /* __FNPROTO_H */
