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

#endif /* __FNPROTO_H */
