/* conf.c - configuration functions and defaults */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by interface */
#include "interface.h"	/* required by code */

#include "command.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "match.h"	/* required by code */
#include "ansi.h"	/* required by code */

/* Some systems are lame, and inet_addr() claims to return -1 on failure,
 * despite the fact that it returns an unsigned long. (It's not really a -1,
 * obviously.) Better-behaved systems use INADDR_NONE.
 */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

/*
 * ---------------------------------------------------------------------------
 * * External symbols.
 */

CONFDATA mudconf;
STATEDATA mudstate;

extern NAMETAB logdata_nametab[];
extern NAMETAB logoptions_nametab[];
extern NAMETAB access_nametab[];
extern NAMETAB attraccess_nametab[];
extern NAMETAB list_names[];
extern NAMETAB sigactions_nametab[];
extern CONF conftable[];
extern LOGFILETAB logfds_table[];

/* ---------------------------------------------------------------------------
 * cf_init: Initialize mudconf to default values.
 */

void NDECL(cf_init)
{
	int i;

	mudconf.dbhome = XSTRDUP(".", "cf_string");
	mudconf.crashdb = XSTRDUP("", "cf_string");
	mudconf.gdbm = XSTRDUP("", "cf_string");
	mudconf.status_file = XSTRDUP("shutdown.status", "cf_string");
	mudstate.modules_list = NULL;
	mudstate.modloaded[0] = '\0';
	mudconf.port = 6250;
	mudconf.conc_port = 6251;
	mudconf.init_size = 1000;
	mudconf.use_global_aconn = 1;
	mudconf.global_aconn_uselocks = 0;
	mudconf.guest_char = NOTHING;
	mudconf.guest_nuker = GOD;
	mudconf.number_guests = 30;
	mudconf.guest_basename = XSTRDUP("Guest", "cf_string");
	mudconf.guest_password = XSTRDUP("guest", "cf_string");
	mudconf.guest_prefixes = XSTRDUP("", "cf_string");
	mudconf.guest_suffixes = XSTRDUP("", "cf_string");
	mudconf.guest_file = XSTRDUP("text/guest.txt", "cf_string");
	mudconf.conn_file = XSTRDUP("text/connect.txt", "cf_string");
	mudconf.creg_file = XSTRDUP("text/register.txt", "cf_string");
	mudconf.regf_file = XSTRDUP("text/create_reg.txt", "cf_string");
	mudconf.motd_file = XSTRDUP("text/motd.txt", "cf_string");
	mudconf.wizmotd_file = XSTRDUP("text/wizmotd.txt", "cf_string");
	mudconf.quit_file = XSTRDUP("text/quit.txt", "cf_string");
	mudconf.down_file = XSTRDUP("text/down.txt", "cf_string");
	mudconf.full_file = XSTRDUP("text/full.txt", "cf_string");
	mudconf.site_file = XSTRDUP("text/badsite.txt", "cf_string");
	mudconf.crea_file = XSTRDUP("text/newuser.txt", "cf_string");
#ifdef PUEBLO_SUPPORT
	mudconf.htmlconn_file = XSTRDUP("text/htmlconn.txt", "cf_string");
#endif
	mudconf.motd_msg = XSTRDUP("", "cf_string");
	mudconf.wizmotd_msg = XSTRDUP("", "cf_string");
	mudconf.downmotd_msg = XSTRDUP("", "cf_string");
	mudconf.fullmotd_msg = XSTRDUP("", "cf_string");
	mudconf.dump_msg = XSTRDUP("", "cf_string");
	mudconf.postdump_msg = XSTRDUP("", "cf_string");
	mudconf.fixed_home_msg = XSTRDUP("", "cf_string");
	mudconf.fixed_tel_msg = XSTRDUP("", "cf_string");
#ifdef PUEBLO_SUPPORT
	mudconf.pueblo_msg = XSTRDUP("</xch_mudtext><img xch_mode=html><tt>", "cf_string");
#endif
	mudconf.sql_host = XSTRDUP("127.0.0.1", "cf_string");
	mudconf.sql_db = XSTRDUP("", "cf_string");
	mudconf.sql_username = XSTRDUP("", "cf_string");
	mudconf.sql_password = XSTRDUP("", "cf_string");
	mudconf.sql_reconnect = 0;
	mudconf.indent_desc = 0;
       	mudconf.name_spaces = 1;
	mudconf.fork_dump = 0;
	mudconf.fork_vfork = 0;
	mudconf.dbopt_interval = 0;
#ifdef PUEBLO_SUPPORT
	mudconf.have_pueblo = 1;
#else
	mudconf.have_pueblo = 0;
#endif
	mudconf.have_zones = 1;
	mudconf.paranoid_alloc = 0;
	mudconf.sig_action = SA_DFLT;
	mudconf.max_players = -1;
	mudconf.dump_interval = 3600;
	mudconf.check_interval = 600;
	mudconf.events_daily_hour = 7;
	mudconf.dump_offset = 0;
	mudconf.check_offset = 300;
	mudconf.idle_timeout = 3600;
	mudconf.conn_timeout = 120;
	mudconf.idle_interval = 60;
	mudconf.retry_limit = 3;
	mudconf.output_limit = 16384;
	mudconf.paycheck = 0;
	mudconf.paystart = 0;
	mudconf.paylimit = 10000;
	mudconf.start_quota = mudconf.start_room_quota =
	    mudconf.start_exit_quota = mudconf.start_thing_quota =
	    mudconf.start_player_quota = 20;
	mudconf.site_chars = 25;
	mudconf.payfind = 0;
	mudconf.digcost = 10;
	mudconf.linkcost = 1;
	mudconf.opencost = 1;
	mudconf.createmin = 10;
	mudconf.createmax = 505;
	mudconf.killmin = 10;
	mudconf.killmax = 100;
	mudconf.killguarantee = 100;
	mudconf.robotcost = 1000;
	mudconf.pagecost = 10;
	mudconf.searchcost = 100;
	mudconf.waitcost = 10;
	mudconf.machinecost = 64;
	mudconf.building_limit = 50000;
	mudconf.exit_quota = 1;
	mudconf.player_quota = 1;
	mudconf.room_quota = 1;
	mudconf.thing_quota = 1;
	mudconf.queuemax = 100;
	mudconf.queue_chunk = 10;
	mudconf.active_q_chunk = 10;
	mudconf.sacfactor = 5;
	mudconf.sacadjust = -1;
	mudconf.use_hostname = 1;
	mudconf.quotas = 0;
	mudconf.typed_quotas = 0;
	mudconf.ex_flags = 1;
	mudconf.robot_speak = 1;
	mudconf.clone_copy_cost = 0;
	mudconf.pub_flags = 1;
	mudconf.quiet_look = 1;
	mudconf.exam_public = 1;
	mudconf.read_rem_desc = 0;
	mudconf.read_rem_name = 0;
	mudconf.sweep_dark = 0;
	mudconf.player_listen = 0;
	mudconf.quiet_whisper = 1;
	mudconf.dark_sleepers = 1;
	mudconf.see_own_dark = 1;
	mudconf.idle_wiz_dark = 0;
	mudconf.visible_wizzes = 0;
	mudconf.pemit_players = 0;
	mudconf.pemit_any = 0;
	mudconf.addcmd_match_blindly = 1;
	mudconf.addcmd_obey_stop = 0;
	mudconf.addcmd_obey_uselocks = 0;
	mudconf.lattr_oldstyle = 0;
	mudconf.bools_oldstyle = 0;
	mudconf.match_mine = 0;
	mudconf.match_mine_pl = 0;
	mudconf.switch_df_all = 1;
	mudconf.fascist_objeval = 0;
	mudconf.fascist_tport = 0;
	mudconf.terse_look = 1;
	mudconf.terse_contents = 1;
	mudconf.terse_exits = 1;
	mudconf.terse_movemsg = 1;
	mudconf.trace_topdown = 1;
	mudconf.trace_limit = 200;
	mudconf.safe_unowned = 0;
	mudconf.wiz_obey_linklock = 0;
	mudconf.local_masters = 1;
	mudconf.req_cmds_flag = 1;
	mudconf.ansi_colors = 1;
	mudconf.safer_passwords = 0;
	mudconf.instant_recycle = 1;
	mudconf.dark_actions = 0;
	mudconf.no_ambiguous_match = 0;
	mudconf.exit_calls_move = 0;
	mudconf.move_match_more = 0;
	mudconf.autozone = 1;
	mudconf.page_req_equals = 0;
	mudconf.comma_say = 0;
	mudconf.you_say = 1;
	
	/* -- ??? Running SC on a non-SC DB may cause problems */
	mudconf.space_compress = 1;

	mudconf.start_room = 0;
	mudconf.guest_start_room = NOTHING; /* default, use start_room */
	mudconf.start_home = NOTHING;
	mudconf.default_home = NOTHING;
	mudconf.master_room = NOTHING;
	mudconf.player_proto = NOTHING;
	mudconf.room_proto = NOTHING;
	mudconf.exit_proto = NOTHING;
	mudconf.thing_proto = NOTHING;
	mudconf.player_defobj = NOTHING;
	mudconf.room_defobj = NOTHING;
	mudconf.thing_defobj = NOTHING;
	mudconf.exit_defobj = NOTHING;
	mudconf.player_parent = NOTHING;
	mudconf.room_parent = NOTHING;
	mudconf.exit_parent = NOTHING;
	mudconf.thing_parent = NOTHING;
	mudconf.player_flags.word1 = 0;
	mudconf.player_flags.word2 = 0;
	mudconf.player_flags.word3 = 0;
	mudconf.room_flags.word1 = 0;
	mudconf.room_flags.word2 = 0;
	mudconf.room_flags.word3 = 0;
	mudconf.exit_flags.word1 = 0;
	mudconf.exit_flags.word2 = 0;
	mudconf.exit_flags.word3 = 0;
	mudconf.thing_flags.word1 = 0;
	mudconf.thing_flags.word2 = 0;
	mudconf.thing_flags.word3 = 0;
	mudconf.robot_flags.word1 = ROBOT;
	mudconf.robot_flags.word2 = 0;
	mudconf.robot_flags.word3 = 0;
	mudconf.stripped_flags.word1 = IMMORTAL | INHERIT | ROYALTY | WIZARD;
	mudconf.stripped_flags.word2 = BLIND | CONNECTED | GAGGED |
	    HEAD_FLAG | SLAVE | STAFF | STOP_MATCH | SUSPECT | UNINSPECTED;
	mudconf.stripped_flags.word3 = 0;
	mudconf.vattr_flags = 0;
	mudconf.vattr_flag_list = NULL;
	mudconf.mud_name = XSTRDUP("TinyMUSH", "cf_string");
	mudconf.one_coin = XSTRDUP("penny", "cf_string");
	mudconf.many_coins = XSTRDUP("pennies", "cf_string");
	mudconf.struct_dstr = XSTRDUP("\r\n", "cf_string");
	mudconf.timeslice = 1000;
	mudconf.cmd_quota_max = 100;
	mudconf.cmd_quota_incr = 1;
#ifdef NO_LAG_CHECK
	mudconf.lag_check = 0;
#else
	mudconf.lag_check = 1;
#endif
	mudconf.max_cmdsecs = 120;
	mudconf.control_flags = 0xffffffff;	/* Everything for now... */
	mudconf.control_flags &= ~CF_GODMONITOR; /* Except for monitoring... */
	mudconf.log_options = LOG_ALWAYS | LOG_BUGS | LOG_SECURITY |
		LOG_NET | LOG_LOGIN | LOG_DBSAVES | LOG_CONFIGMODS |
		LOG_SHOUTS | LOG_STARTUP | LOG_WIZARD |
		LOG_PROBLEMS | LOG_PCREATES | LOG_TIMEUSE | LOG_LOCAL;
	mudconf.log_info = LOGOPT_TIMESTAMP | LOGOPT_LOC;
	mudconf.log_diversion = 0;
	mudconf.markdata[0] = 0x01;
	mudconf.markdata[1] = 0x02;
	mudconf.markdata[2] = 0x04;
	mudconf.markdata[3] = 0x08;
	mudconf.markdata[4] = 0x10;
	mudconf.markdata[5] = 0x20;
	mudconf.markdata[6] = 0x40;
	mudconf.markdata[7] = 0x80;
	mudconf.cmd_nest_lim = 50;
	mudconf.cmd_invk_lim = 2500;
	mudconf.func_nest_lim = 50;
	mudconf.func_invk_lim = 2500;
	mudconf.func_cpu_lim_secs = 60;
	mudconf.func_cpu_lim = 60 * CLOCKS_PER_SEC;
	mudconf.ntfy_nest_lim = 20;
	mudconf.fwdlist_lim = 100;
	mudconf.lock_nest_lim = 20;
	mudconf.parent_nest_lim = 10;
	mudconf.zone_nest_lim = 20;
	mudconf.numvars_lim = 50;
	mudconf.stack_lim = 50;
	mudconf.struct_lim = 100;
	mudconf.instance_lim = 100;
	mudconf.max_player_aliases = 10;
	mudconf.cache_width = CACHE_WIDTH;
	mudconf.cache_size = CACHE_SIZE;

	mudstate.events_flag = 0;
	mudstate.initializing = 0;
	mudstate.loading_db = 0;
	mudstate.panicking = 0;
	mudstate.standalone = 0;
	mudstate.dumping = 0;
	mudstate.logging = 0;
	mudstate.epoch = 0;
	mudstate.generation = 0;
	mudstate.reboot_nums = 0;
	mudstate.mudlognum = 0;
	mudstate.helpfiles = 0;
	mudstate.hfiletab = NULL;
	mudstate.hfiletab_size = 0;
	mudstate.hfile_hashes = NULL;
	mudstate.curr_player = NOTHING;
	mudstate.curr_enactor = NOTHING;
	mudstate.curr_cmd = (char *) "< none >";
    	mudstate.shutdown_flag = 0;
    	mudstate.flatfile_flag = 0;
	mudstate.attr_next = A_USER_START;
	mudstate.debug_cmd = (char *)"< init >";
	StringCopy(mudstate.doing_hdr, "Doing");
	mudstate.access_list = NULL;
	mudstate.suspect_list = NULL;
	mudstate.qfirst = NULL;
	mudstate.qlast = NULL;
	mudstate.qlfirst = NULL;
	mudstate.qllast = NULL;
	mudstate.qwait = NULL;
	mudstate.qsemfirst = NULL;
	mudstate.qsemlast = NULL;
	mudstate.badname_head = NULL;
	mudstate.mstat_ixrss[0] = 0;
	mudstate.mstat_ixrss[1] = 0;
	mudstate.mstat_idrss[0] = 0;
	mudstate.mstat_idrss[1] = 0;
	mudstate.mstat_isrss[0] = 0;
	mudstate.mstat_isrss[1] = 0;
	mudstate.mstat_secs[0] = 0;
	mudstate.mstat_secs[1] = 0;
	mudstate.mstat_curr = 0;
	mudstate.iter_alist.data = NULL;
	mudstate.iter_alist.len = 0;
	mudstate.iter_alist.next = NULL;
	mudstate.mod_alist = NULL;
	mudstate.mod_size = 0;
	mudstate.mod_al_id = NOTHING;
	mudstate.olist = NULL;
	mudstate.min_size = 0;
	mudstate.db_top = 0;
	mudstate.db_size = 0;
	mudstate.moduletype_top = DBTYPE_RESERVED;
	mudstate.freelist = NOTHING;
	mudstate.markbits = NULL;
	mudstate.sql_socket = -1;
	mudstate.cmd_nest_lev = 0;
	mudstate.cmd_invk_ctr = 0;
	mudstate.func_nest_lev = 0;
	mudstate.func_invk_ctr = 0;
	mudstate.cputime_base = clock();
	mudstate.ntfy_nest_lev = 0;
	mudstate.lock_nest_lev = 0;
	mudstate.zone_nest_num = 0;
	mudstate.in_loop = 0;
	mudstate.loop_token[0] = NULL;
	mudstate.loop_number[0] = 0;
	mudstate.in_switch = 0;
	mudstate.switch_token = NULL;
	mudstate.inpipe = 0;
	mudstate.pout = NULL;
	mudstate.poutnew = NULL;
	mudstate.poutbufc = NULL;
	mudstate.poutobj = -1;
	mudstate.dbm_fd = -1;
	mudstate.rdata = NULL;
}

/* ---------------------------------------------------------------------------
 * cf_log_notfound: Log a 'parameter not found' error.
 */

void cf_log_notfound(player, cmd, thingname, thing)
dbref player;
char *cmd, *thing;
const char *thingname;
{
	if (mudstate.initializing) {
		STARTLOG(LOG_STARTUP, "CNF", "NFND")
		    log_printf("%s: %s %s not found", cmd, thingname, thing);
		ENDLOG
	} else {
		notify(player, tprintf("%s %s not found", thingname, thing));
	}
}

/* ---------------------------------------------------------------------------
 * cf_log_syntax: Log a syntax error.
 */

#if defined(__STDC__) && defined(STDC_HEADERS)
void cf_log_syntax(dbref player, char *cmd, const char *template,...)
#else
void cf_log_syntax(va_alist)
va_dcl
#endif
{
	va_list ap;

#if defined(__STDC__) && defined(STDC_HEADERS)
	va_start(ap, template);
#else
	dbref player;
	char *cmd, *template;

	player = va_arg(ap, dbref);
	cmd = va_arg(ap, char *);
	template = va_arg(ap, char *);
#endif

	if (mudstate.initializing) {
		STARTLOG(LOG_STARTUP, "CNF", "SYNTX")
		    log_printf("%s: ", cmd);
		    log_vprintf(template, ap);
		ENDLOG
	} else {
		notify(player, tvprintf(template, ap));
	}

	va_end(ap);
}

/* ---------------------------------------------------------------------------
 * cf_status_from_succfail: Return command status from succ and fail info
 */

int cf_status_from_succfail(player, cmd, success, failure)
dbref player;
char *cmd;
int success, failure;
{
	/*
	 * If any successes, return SUCCESS(0) if no failures or
	 * PARTIAL_SUCCESS(1) if any failures. 
	 */

	if (success > 0)
		return ((failure == 0) ? 0 : 1);

	/*
	 * No successes.  If no failures indicate nothing done. Always return 
	 * FAILURE(-1) 
	 */

	if (failure == 0) {
		if (mudstate.initializing) {
			STARTLOG(LOG_STARTUP, "CNF", "NDATA")
			    log_printf("%s: Nothing to set", cmd);
			ENDLOG
		} else {
			notify(player, "Nothing to set");
		}
	}
	return -1;
}

/* ---------------------------------------------------------------------------
 * cf_const: Read-only integer or boolean parameter.
 */

CF_HAND(cf_const)
{
	/* Fail on any attempt to change the value */

	return -1;
}

/* ---------------------------------------------------------------------------
 * cf_int: Set integer parameter.
 */

CF_HAND(cf_int)
{
	/* Copy the numeric value to the parameter */

	if ((extra > 0) && (atoi(str) > extra)) {
	    cf_log_syntax(player, cmd, "Value exceeds limit of %d", extra);
	    return -1;
	}
	sscanf(str, "%d", vp);
	return 0;
}

/* ---------------------------------------------------------------------------
 * cf_dbref: Set dbref parameter.
 */

CF_HAND(cf_dbref)
{
    int num;

    /* No consistency check on initialization. */

    if (mudstate.initializing) {
	if (*str == '#') {
	    sscanf(str, "#%d", vp);
	} else {
	    sscanf(str, "%d", vp);
	}
	return 0;
    }

    /* Otherwise we have to validate this. If 'extra' is non-zero, the
     * dbref is allowed to be NOTHING.
     */

    if (*str == '#')
	num = atoi(str + 1);
    else
	num = atoi(str);

    if (((extra == NOTHING) && (num == NOTHING)) ||
	(Good_obj(num) && !Going(num))) {
	if (*str == '#') {
	    sscanf(str, "#%d", vp);
	} else {
	    sscanf(str, "%d", vp);
	}
	return 0;
    }

    if (extra == NOTHING) {
	cf_log_syntax(player, cmd, "A valid dbref, or -1, is required.");
    } else {
	cf_log_syntax(player, cmd, "A valid dbref is required.");
    }
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_module: Open a loadable module. Modules are initialized later in the
 * startup.
 */

CF_HAND(cf_module)
{
	lt_dlhandle handle;
	void (*initptr)(void);
	MODULE *mp;
	
	handle = lt_dlopen(tprintf("%s.la", str));

	if (!handle) {
		STARTLOG(LOG_STARTUP, "CNF", "MOD")
		    log_printf("Loading of %s module failed: %s",
			       str, lt_dlerror());
		ENDLOG
		return -1;
	}

	mp = (MODULE *) XMALLOC(sizeof(MODULE), "cf_module.mp");
	mp->modname = XSTRDUP(str, "cf_module.name");
	mp->handle = handle;
	mp->next = mudstate.modules_list;
	mudstate.modules_list = mp;

	/* Look up our symbols now, and cache the pointers. They're
	 * not going to change from here on out.
	 */

	mp->process_command = DLSYM_INT(handle, str, "process_command",
					(dbref, dbref, int, char *,
					 char *[], int));
	mp->process_no_match = DLSYM_INT(handle, str, "process_no_match",
					(dbref, dbref, int, char *, char *,
					 char *[], int));
	mp->did_it = DLSYM_INT(handle, str, "did_it",
			       (dbref, dbref, dbref,
				int, const char *, int, const char *, int,
				int, char *[], int, int));
	mp->create_obj = DLSYM(handle, str, "create_obj", (dbref, dbref));
	mp->destroy_obj = DLSYM(handle, str, "destroy_obj", (dbref, dbref));
	mp->create_player = DLSYM(handle, str, "create_player",
				  (dbref, dbref, int, int));
	mp->destroy_player = DLSYM(handle, str, "destroy_player",
				   (dbref, dbref));
	mp->announce_connect = DLSYM(handle, str, "announce_connect",
				     (dbref, const char *, int));
	mp->announce_disconnect = DLSYM(handle, str, "announce_disconnect",
					(dbref, const char *, int));
	mp->examine = DLSYM(handle, str, "examine",
			    (dbref, dbref, dbref, int, int));
	mp->dump_database = DLSYM(handle, str, "dump_database", (FILE *));
	mp->db_grow = DLSYM(handle, str, "db_grow", (int, int));
	mp->db_write = DLSYM(handle, str, "db_write", (void));
	mp->db_write_flatfile = DLSYM(handle, str, "db_write_flatfile", (FILE *));
	mp->do_second = DLSYM(handle, str, "do_second", (void));
	mp->cache_put_notify = DLSYM(handle, str, "cache_put_notify", (DBData, unsigned int));
	mp->cache_del_notify = DLSYM(handle, str, "cache_del_notify", (DBData, unsigned int));

	if (!mudstate.standalone) {
		if ((initptr = DLSYM(handle, str, "init", (void))) != NULL)
		    (*initptr)();
	}
	
	STARTLOG(LOG_STARTUP, "CNF", "MOD")
		log_printf("Loaded module: %s", str);
	ENDLOG

	return 0;
}

/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * cf_bool: Set boolean parameter.
 */

NAMETAB bool_names[] = {
{(char *)"true",	1,	0,	1},
{(char *)"false",	1,	0,	0},
{(char *)"yes",		1,	0,	1},
{(char *)"no",		1,	0,	0},
{(char *)"1",		1,	0,	1},
{(char *)"0",		1,	0,	0},
{NULL,			0,	0,	0}};

/* *INDENT-ON* */


CF_HAND(cf_bool)
{
	*vp = (int) search_nametab(GOD, bool_names, str);
	if (*vp < 0)
		*vp = (long) 0;
	return 0;
}

/* ---------------------------------------------------------------------------
 * cf_option: Select one option from many choices.
 */

CF_HAND(cf_option)
{
	int i;

	i = search_nametab(GOD, (NAMETAB *) extra, str);
	if (i < 0) {
		cf_log_notfound(player, cmd, "Value", str);
		return -1;
	}
	*vp = i;
	return 0;
}

/* ---------------------------------------------------------------------------
 * cf_string: Set string parameter.
 */

CF_HAND(cf_string)
{
	int retval;

	/*
	 * Make a copy of the string if it is not too big 
	 */

	retval = 0;
	if (strlen(str) >= (unsigned int)extra) {
		str[extra - 1] = '\0';
		if (mudstate.initializing) {
			STARTLOG(LOG_STARTUP, "CNF", "NFND")
			log_printf("%s: String truncated", cmd);
			ENDLOG
		} else {
			notify(player, "String truncated");
		}
		retval = 1;
	}
        XFREE(*(char **)vp, "cf_string");
	*(char **)vp = XSTRDUP(str, "cf_string");
	return retval;
}

/* ---------------------------------------------------------------------------
 * cf_alias: define a generic hash table alias.
 */

CF_HAND(cf_alias)
{
	char *alias, *orig, *p, *tokst;
	int *cp, upcase;

	alias = strtok_r(str, " \t=,", &tokst);
	orig = strtok_r(NULL, " \t=,", &tokst);
	if (orig) {
		upcase = 0;
		for (p = orig; *p; p++)
			*p = tolower(*p);
		cp = hashfind(orig, (HASHTAB *) vp);
		if (cp == NULL) {
			upcase++;
			for (p = orig; *p; p++)
				*p = toupper(*p);
			cp = hashfind(orig, (HASHTAB *) vp);
			if (cp == NULL) {
				cf_log_notfound(player, cmd,
						(char *) extra, orig);
				return -1;
			}
		}
		if (upcase) {
			for (p = alias; *p; p++)
				*p = toupper(*p);
		} else {
			for (p = alias; *p; p++)
				*p = tolower(*p);
		}
		if (((HASHTAB *)vp)->flags & HT_KEYREF) {
			/* hashadd won't copy it, so we do that here */
			p = alias;
			alias = XSTRDUP(p, "cf_alias");
		}
		return hashadd(alias, cp, (HASHTAB *) vp, HASH_ALIAS);
	} else {
		cf_log_syntax(player, cmd, "Invalid original for alias %s",
			      alias);
		return -1;
	}
}

/* ---------------------------------------------------------------------------
 * cf_divert_log: Redirect a log type.
 */

CF_HAND(cf_divert_log)
{
    char *type_str, *file_str, *tokst;
    int f, fd;
    FILE *fptr;
    LOGFILETAB *tp, *lp;

    /* Two args, two args only */

    type_str = strtok_r(str, " \t", &tokst);
    file_str = strtok_r(NULL, " \t", &tokst);
    if (!type_str || !file_str) {
	cf_log_syntax(player, cmd, "Missing pathname to log to.", (char *) "");
	return -1;
    }

    /* Find the log. */

    f = search_nametab(GOD, (NAMETAB *) extra, type_str);
    if (f <= 0) {
	cf_log_notfound(player, cmd, "Log diversion", type_str);
	return -1;
    }

    for (tp = logfds_table; tp->log_flag; tp++) {
	if (tp->log_flag == f)
	    break;
    }
    if (tp == NULL) {		/* This should never happen! */
	cf_log_notfound(player, cmd, "Logfile table corruption", type_str);
	return -1;
    }

    /* We shouldn't have a file open already. */

    if (tp->filename != NULL) {
	    STARTLOG(LOG_STARTUP, "CNF", "DIVT")
		log_printf("Log type %s already diverted: %s",
			   type_str, tp->filename);
	    ENDLOG
	    return -1;
    }

    /* Check to make sure that we don't have this filename open already. */

    fptr = NULL;
    for (lp = logfds_table; lp->log_flag; lp++) {
	if (lp->filename && !strcmp(file_str, lp->filename)) {
	    fptr = lp->fileptr;
	    break;
	}
    }

    /* We don't have this filename yet. Open the logfile. */

    if (!fptr) {
	fptr = fopen(file_str, "w");
	if (!fptr) {
	    STARTLOG(LOG_STARTUP, "CNF", "DIVT")
		log_printf("Cannot open logfile: %s", file_str);
	    ENDLOG
	    return -1;
	}
	
	if ((fd = fileno(fptr)) == -1) {
		return -1;
	}
	
#ifdef FNDELAY
	if (fcntl(fd, F_SETFL, FNDELAY) == -1) {
	    STARTLOG(LOG_STARTUP, "CNF", "DIVT")
		log_printf("Cannot make nonblocking: %s", file_str);
	    ENDLOG
	    return -1;
	}
#else
	if (fcntl(fd, F_SETFL, O_NDELAY) == -1) {
	    STARTLOG(LOG_STARTUP, "CNF", "DIVT")
		log_printf("Cannot make nonblocking: %s", file_str);
	    ENDLOG
	    return -1;
	}
#endif
    }

    /* Indicate that this is being diverted. */

    tp->fileptr = fptr;
    tp->filename = XSTRDUP(file_str, "cf_divert_log");
    *vp |= f;

    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_modify_bits: set or clear bits in a flag word from a namelist.
 */

CF_HAND(cf_modify_bits)
{
	char *sp, *tokst;
	int f, negate, success, failure;

	/*
	 * Walk through the tokens 
	 */

	success = failure = 0;
	sp = strtok_r(str, " \t", &tokst);
	while (sp != NULL) {

		/* Check for negation */

		negate = 0;
		if (*sp == '!') {
			negate = 1;
			sp++;
		}

		/* Set or clear the appropriate bit */

		f = search_nametab(GOD, (NAMETAB *) extra, sp);
		if (f > 0) {
			if (negate)
				*vp &= ~f;
			else
				*vp |= f;
			success++;
		} else {
			cf_log_notfound(player, cmd, "Entry", sp);
			failure++;
		}

		/* Get the next token */

		sp = strtok_r(NULL, " \t", &tokst);
	}
	return cf_status_from_succfail(player, cmd, success, failure);
}

/* ---------------------------------------------------------------------------
 * modify_xfuncs: Helper function to change xfuncs.
 */

static NAMEDFUNC **all_named_funcs = NULL;
static int num_named_funcs = 0;

static int modify_xfuncs(fn_name, fn_ptr, xfuncs, negate)
    char *fn_name;
    int (*fn_ptr)(dbref);
    EXTFUNCS **xfuncs;
    int negate;		/* 0 - normal, 1 - remove */
{
    EXTFUNCS *xfp;
    NAMEDFUNC *np, **tp;
    int i;

    xfp = *xfuncs;

    /* If we're negating, just remove it from the list of functions. */

    if (negate) {
	if (!xfp)
	    return 0;
	for (i = 0; i < xfp->num_funcs; i++) {
	    if (!strcmp(xfp->ext_funcs[i]->fn_name, fn_name)) {
		xfp->ext_funcs[i] = NULL;
		return 1;
	    }
	}
	return 0;
    }

    /* Have we encountered this function before? */

    np = NULL;
    for (i = 0; i < num_named_funcs; i++) {
	if (!strcmp(all_named_funcs[i]->fn_name, fn_name)) {
	    np = all_named_funcs[i];
	    break;
	}
    }

    /* If not, we need to allocate it. */

    if (!np) {
	np = (NAMEDFUNC *) XMALLOC(sizeof(NAMEDFUNC), "nfunc");
	np->fn_name = (char *) strdup(fn_name);
	np->handler = fn_ptr;
    }

    /* Add it to the ones we know about. */

    if (num_named_funcs == 0) {
	all_named_funcs = (NAMEDFUNC **) XMALLOC(sizeof(NAMEDFUNC *),
						 "all_named_funcs");
	num_named_funcs = 1;
	all_named_funcs[0] = np;
    } else {
	tp = (NAMEDFUNC **) XREALLOC(all_named_funcs,
				(num_named_funcs + 1) * sizeof(NAMEDFUNC *),
				"all_named_funcs");
	tp[num_named_funcs] = np;
	all_named_funcs = tp;
	num_named_funcs++;
    }

    /* Do we have an existing list of functions? If not, this is easy. */

    if (!xfp) {
	xfp = (EXTFUNCS *) XMALLOC(sizeof(EXTFUNCS), "xfunc");
	xfp->num_funcs = 1;
	xfp->ext_funcs = (NAMEDFUNC **) XMALLOC(sizeof(NAMEDFUNC *), "nfuncs");
	xfp->ext_funcs[0] = np;
	*xfuncs = xfp;
	return 1;
    }

    /* See if we have an empty slot to insert into. */

    for (i = 0; i < xfp->num_funcs; i++) {
	if (!xfp->ext_funcs[i]) {
	    xfp->ext_funcs[i] = np;
	    return 1;
	}
    }

    /* Guess not. Tack it onto the end. */

    tp = (NAMEDFUNC **) XREALLOC(xfp->ext_funcs,
				 (xfp->num_funcs + 1) * sizeof(NAMEDFUNC *),
				 "nfuncs");
    tp[xfp->num_funcs] = np;
    xfp->ext_funcs = tp;
    xfp->num_funcs++;
    return 1;
}

/* ---------------------------------------------------------------------------
 * parse_ext_access: Parse an extended access list with module callouts.
 */

int parse_ext_access(perms, xperms, str, ntab, player, cmd)
    int *perms;
    EXTFUNCS **xperms;
    char *str;
    NAMETAB *ntab;
    dbref player;
    char *cmd;
{
    char *sp, *tokst, *cp, *ostr;
    int f, negate, success, failure, got_one;
    MODULE *mp;
    int (*hp)(dbref);

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);
    while (sp != NULL) {

	/* Check for negation  */

	negate = 0;
	if (*sp == '!') {
	    negate = 1;
	    sp++;
	}

	/* Set or clear the appropriate bit */

	f = search_nametab(GOD, ntab, sp);
	if (f > 0) {
	    if (negate)
		*perms &= ~f;
	    else
		*perms |= f;
	    success++;
	} else {

	    /* Is this a module callout? */

	    got_one = 0;

	    if (!strncmp(sp, "mod_", 4)) {

		/* Split it apart, see if we have anything. */

		ostr = (char *) strdup(sp); 
		if (*(sp + 4) != '\0') {
		    cp = strchr(sp + 4, '_');
		    if (cp) {
			*cp++ = '\0';
			WALK_ALL_MODULES(mp) {
			    got_one = 1;
			    if (!strcmp(sp + 4, mp->modname)) {
				hp = DLSYM_INT(mp->handle, mp->modname, cp,
					       (dbref));
				if (!hp) {
				    cf_log_notfound(player, cmd,
						   "Module function", ostr);
				    failure++;
				} else {
				    if (modify_xfuncs(ostr, hp, xperms,
						      negate))
					success++;
				    else
					failure++;
				}
				break;
			    }
			}
			if (!got_one) {
			    cf_log_notfound(player, cmd, "Loaded module",
					    sp + 4);
			    got_one = 1;
			}
		    }
		}
		free(ostr);
	    }

	    if (!got_one) {
		cf_log_notfound(player, cmd, "Entry", sp);
		failure++;
	    }
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/* ---------------------------------------------------------------------------
 * cf_set_flags: Clear flag word and then set from a flags htab.
 */

CF_HAND(cf_set_flags)
{
	char *sp, *tokst;
	FLAGENT *fp;
	FLAGSET *fset;

	int success, failure;

	for (sp = str; *sp; sp++)
		*sp = toupper(*sp);

	/*
	 * Walk through the tokens 
	 */

	success = failure = 0;
	sp = strtok_r(str, " \t", &tokst);
	fset = (FLAGSET *) vp;

	while (sp != NULL) {

		/*
		 * Set the appropriate bit 
		 */

		fp = (FLAGENT *) hashfind(sp, &mudstate.flags_htab);
		if (fp != NULL) {
			if (success == 0) {
				(*fset).word1 = 0;
				(*fset).word2 = 0;
				(*fset).word3 = 0;
			}
			if (fp->flagflag & FLAG_WORD3)
				(*fset).word3 |= fp->flagvalue;
			else if (fp->flagflag & FLAG_WORD2)
				(*fset).word2 |= fp->flagvalue;
			else
				(*fset).word1 |= fp->flagvalue;
			success++;
		} else {
			cf_log_notfound(player, cmd, "Entry", sp);
			failure++;
		}

		/*
		 * Get the next token 
		 */

		sp = strtok_r(NULL, " \t", &tokst);
	}
	if ((success == 0) && (failure == 0)) {
		(*fset).word1 = 0;
		(*fset).word2 = 0;
		(*fset).word3 = 0;
		return 0;
	}
	if (success > 0)
		return ((failure == 0) ? 0 : 1);
	return -1;
}

/* ---------------------------------------------------------------------------
 * cf_badname: Disallow use of player name/alias.
 */

CF_HAND(cf_badname)
{
	if (extra)
		badname_remove(str);
	else
		badname_add(str);
	return 0;
}

/* ---------------------------------------------------------------------------
 * sane_inet_addr: inet_addr() does not necessarily do reasonable checking
 * for sane syntax. On certain operating systems, if passed less than four
 * octets, it will cause a segmentation violation. This is unfriendly.
 * We take steps here to deal with it.
 */

static unsigned long sane_inet_addr(str)
    char *str;
{
    int i;
    char *p;

    p = str;
    for (i = 1; (p = strchr(p, '.')) != NULL; i++, p++)
	;
    if (i < 4)
	return INADDR_NONE;
    else
	return inet_addr(str);
}


/* ---------------------------------------------------------------------------
 * cf_site: Update site information
 */

CF_AHAND(cf_site)
{
    SITE *site, *last, *head;
    char *addr_txt, *mask_txt, *tokst;
    struct in_addr addr_num, mask_num;
    int mask_bits;

    if ((mask_txt = strchr(str, '/')) == NULL) {

	/* Standard IP range and netmask notation. */

	addr_txt = strtok_r(str, " \t=,", &tokst);
	if (addr_txt)
	    mask_txt = strtok_r(NULL, " \t=,", &tokst);
	if (!addr_txt || !*addr_txt || !mask_txt || !*mask_txt) {
	    cf_log_syntax(player, cmd, "Missing host address or mask.",
			  (char *)"");
	    return -1;
	}
	if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE) {
	    cf_log_syntax(player, cmd, "Malformed host address: %s", addr_txt);
	    return -1;
	}
	if (((mask_num.s_addr = sane_inet_addr(mask_txt)) == INADDR_NONE) &&
	    strcmp(mask_txt, "255.255.255.255")) {
	    cf_log_syntax(player, cmd, "Malformed mask address: %s", mask_txt);
	    return -1;
	}
    } else {

	/* RFC 1517, 1518, 1519, 1520: CIDR IP prefix notation */

	addr_txt = str;
	*mask_txt++ = '\0';
	mask_bits = atoi(mask_txt);
	if ((mask_bits > 32) || (mask_bits < 0)) {
	    cf_log_syntax(player, cmd,
			  "Mask bits (%d) in CIDR IP prefix out of range.",
			  mask_bits);
	    return -1;
	} else if (mask_bits == 0) {
	    mask_num.s_addr = htonl(0); /* can't shift by 32 */
	} else {
	    mask_num.s_addr = htonl(0xFFFFFFFFU << (32 - mask_bits));
	}

	if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE) {
	    cf_log_syntax(player, cmd, "Malformed host address: %s", addr_txt);
	    return -1;
	}
    }

    head = (SITE *) * vp;
    /*
     * Parse the access entry and allocate space for it 
     */

    if (!(site = (SITE *) XMALLOC(sizeof(SITE), "cf_site")))
	return -1;

    /*
     * Initialize the site entry 
     */

    site->address.s_addr = addr_num.s_addr;
    site->mask.s_addr = mask_num.s_addr;
    site->flag = (long) extra;
    site->next = NULL;

    /*
     * Link in the entry.  Link it at the start if not initializing, at
     * the end if initializing.  This is so that entries in the config
     * file are processed as you would think they would be, while
     * entries made while running are processed first. 
     */

    if (mudstate.initializing) {
	if (head == NULL) {
	    *vp = (long *) site;
	} else {
	    for (last = head; last->next; last = last->next)
		;
	    last->next = site;
	}
    } else {
	site->next = head;
	*vp = (long *) site;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_cf_access: Set write or read access on config directives
 * kludge: this cf handler uses vp as an extra extra field since the
 * first extra field is taken up with the access nametab.
 */

static int helper_cf_cf_access(tp, player, vp, ap, cmd, extra)
    CONF *tp;
    dbref player;
    int *vp;
    char *ap, *cmd;
    long extra;
{
    /* Cannot modify parameters set STATIC */

    if (tp->flags & CA_STATIC) {
	notify(player, NOPERM_MESSAGE);
	STARTLOG(LOG_CONFIGMODS, "CFG", "PERM")
	    log_name(player);
	    log_printf(" tried to change %s access to static param: %s",
		       (((long)vp) ? "read" : "write"),
		       tp->pname);
	ENDLOG
	return -1;
    }
    if ((long)vp) {
	return (cf_modify_bits(&tp->rperms, ap, extra,
			       player, cmd));
    } else {
	return (cf_modify_bits(&tp->flags, ap, extra,
			       player, cmd));
    }
}

CF_HAND(cf_cf_access)
{
	CONF *tp, *ctab;
	char *ap;
	MODULE *mp;

	for (ap = str; *ap && !isspace(*ap); ap++) ;
	if (*ap)
		*ap++ = '\0';

	for (tp = conftable; tp->pname; tp++) {
	    if (!strcmp(tp->pname, str)) {
		return (helper_cf_cf_access(tp, player, vp, ap, cmd, extra)); 
	    }
	}

	WALK_ALL_MODULES(mp) {
	    if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
				  CONF *)) != NULL) {
		for (tp = ctab; tp->pname; tp++) {
		    if (!strcmp(tp->pname, str)) {
			return (helper_cf_cf_access(tp, player, vp, ap,
						    cmd, extra));
		    }
		}
	    }
	}

	cf_log_notfound(player, cmd, "Config directive", str);
	return -1;
}

/* ---------------------------------------------------------------------------
 * cf_helpfile: Add a help/news-style file. Only valid during startup.
 */

int add_helpfile(player, confcmd, str, is_raw)
    dbref player;
    char *confcmd, *str;
    int is_raw;
{
    char *fcmd, *fpath, *newstr, *tokst;
    CMDENT *cmdp;
    char **ftab;		/* pointer to an array of filepaths */
    HASHTAB *hashes;

    /* Make a new string so we won't SEGV if given a constant string */
    
    newstr = alloc_mbuf("add_helpfile");
    StringCopy(newstr, str);

    fcmd = strtok_r(newstr, " \t=,", &tokst);
    fpath = strtok_r(NULL, " \t=,", &tokst);
    if (fpath == NULL) {
	cf_log_syntax(player, confcmd, "Missing path for helpfile %s", fcmd);
	free_mbuf(newstr);
	return -1;
    }
    if (fcmd[0] == '_' && fcmd[1] == '_') {
	cf_log_syntax(player, confcmd, "Helpfile %s would cause @addcommand conflict", fcmd);
	free_mbuf(newstr);
	return -1;
    }
    if (strlen(fpath) > SBUF_SIZE) {
	cf_log_syntax(player, confcmd, "Helpfile %s filename too long", fcmd);
	free_mbuf(newstr);
	return -1;
    }
    
    cmdp = (CMDENT *) XMALLOC(sizeof(CMDENT), "add_helpfile.cmdp");
    cmdp->cmdname = XSTRDUP(fcmd, "add_helpfile.cmd");
    cmdp->switches = NULL;
    cmdp->perms = 0;
    cmdp->pre_hook = NULL;
    cmdp->post_hook = NULL;
    cmdp->userperms = NULL;
    cmdp->callseq = CS_ONE_ARG;
    cmdp->info.handler = do_help;

    cmdp->extra = mudstate.helpfiles;
    if (is_raw)
	cmdp->extra |= HELP_RAWHELP;

    hashdelete(cmdp->cmdname, &mudstate.command_htab);
    hashadd(cmdp->cmdname, (int *) cmdp, &mudstate.command_htab, 0);
    hashdelete(tprintf("__%s", cmdp->cmdname), &mudstate.command_htab);
    hashadd(tprintf("__%s", cmdp->cmdname), (int *) cmdp, &mudstate.command_htab, HASH_ALIAS);

    /* We may need to grow the helpfiles table, or create it. */

    if (!mudstate.hfiletab) {

	mudstate.hfiletab = (char **) XCALLOC(4, sizeof(char *), "helpfile.htab");
	mudstate.hfile_hashes = (HASHTAB *) XCALLOC(4, sizeof(HASHTAB), "helpfile.hashes");
	mudstate.hfiletab_size = 4;

    } else if (mudstate.helpfiles >= mudstate.hfiletab_size) {

	ftab = (char **) XREALLOC(mudstate.hfiletab,
				(mudstate.hfiletab_size + 4) * sizeof(char *),
				  "helpfile.htab");
	hashes = (HASHTAB *) XREALLOC(mudstate.hfile_hashes,
				      (mudstate.hfiletab_size + 4) *
				      sizeof(HASHTAB), "helpfile.hashes");
	ftab[mudstate.hfiletab_size] = NULL;
	ftab[mudstate.hfiletab_size + 1] = NULL;
	ftab[mudstate.hfiletab_size + 2] = NULL;
	ftab[mudstate.hfiletab_size + 3] = NULL;
	mudstate.hfiletab_size += 4;
	mudstate.hfiletab = ftab;
	mudstate.hfile_hashes = hashes;

    }

    /* Add or replace the path to the file. */

    if (mudstate.hfiletab[mudstate.helpfiles] != NULL)
	XFREE(mudstate.hfiletab[mudstate.helpfiles], "add_helpfile.fpath");
    mudstate.hfiletab[mudstate.helpfiles] = XSTRDUP(fpath, "add_helpfile.fpath");

    /* Initialize the associated hashtable. */

    hashinit(&mudstate.hfile_hashes[mudstate.helpfiles], 30 * HASH_FACTOR,
	     HT_STR);

    mudstate.helpfiles++;
    free_mbuf(newstr);

    return 0;
}

CF_HAND(cf_helpfile)
{
    return add_helpfile(player, cmd, str, 0);
}

CF_HAND(cf_raw_helpfile)
{
    return add_helpfile(player, cmd, str, 1);
}

/* ---------------------------------------------------------------------------
 * cf_include: Read another config file.  Only valid during startup.
 */

CF_HAND(cf_include)
{
	FILE *fp;
	char *cp, *ap, *zp, *buf;

	extern int FDECL(cf_set, (char *, char *, dbref));


	if (!mudstate.initializing)
		return -1;

	fp = fopen(str, "r");
	if (fp == NULL) {
		cf_log_notfound(player, cmd, "Config file", str);
		return -1;
	}
	buf = alloc_lbuf("cf_include");
	fgets(buf, LBUF_SIZE, fp);
	while (!feof(fp)) {
		cp = buf;
		if (*cp == '#') {
			fgets(buf, LBUF_SIZE, fp);
			continue;
		}
		/*
		 * Not a comment line.  Strip off the NL and any characters
		 * following it.  Then, split the line into the command and
		 * argument portions (separated by a space).  Also, trim off
		 * the trailing comment, if any (delimited by #) 
		 */

		for (cp = buf; *cp && *cp != '\n'; cp++) ;
		*cp = '\0';	/*
				 * strip \n
				 */
		for (cp = buf; *cp && isspace(*cp); cp++) ;	/*
								 * strip
								 * spaces  
								 */
		for (ap = cp; *ap && !isspace(*ap); ap++) ;	/*
								 * skip over
								 * command 
								 */
		if (*ap)
			*ap++ = '\0';	/*
					 * trim command
					 */
		for (; *ap && isspace(*ap); ap++) ;	/*
							 * skip spaces 
							 */
		for (zp = ap; *zp && (*zp != '#'); zp++) ;	/*
								 * find
								 * comment
								 */
		if (*zp && !(isdigit(*(zp + 1)) && isspace(*(zp -1))))
			*zp = '\0';	/*
					 * zap comment, but only if it's
					 * not sitting between whitespace
					 * and a digit, which traps a case
					 * like 'master_room #2'
					 */
		for (zp = zp - 1; zp >= ap && isspace(*zp); zp--)
			*zp = '\0';	/*
					 * zap trailing spcs
					 */

		cf_set(cp, ap, player);
		fgets(buf, LBUF_SIZE, fp);
	}
	free_lbuf(buf);
	fclose(fp);
	return 0;
}

extern CF_HDCL(cf_access);
extern CF_HDCL(cf_cmd_alias);
extern CF_HDCL(cf_acmd_access);
extern CF_HDCL(cf_attr_access);
extern CF_HDCL(cf_attr_type);
extern CF_HDCL(cf_func_access);
extern CF_HDCL(cf_flag_access);
extern CF_HDCL(cf_flag_name);
extern CF_HDCL(cf_power_access);

/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * conftable: Table for parsing the configuration file.
 */

CONF conftable[] = {
{(char *)"access",			cf_access,	CA_GOD,		CA_DISABLED,	NULL,				(long)access_nametab},
{(char *)"addcommands_match_blindly",	cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.addcmd_match_blindly,	(long)"@addcommands don't error if no match is found"},
{(char *)"addcommands_obey_stop",	cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.addcmd_obey_stop,	(long)"@addcommands obey STOP"},
{(char *)"addcommands_obey_uselocks",	cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.addcmd_obey_uselocks,	(long)"@addcommands obey UseLocks"},
{(char *)"alias",			cf_cmd_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.command_htab,	0},
{(char *)"ansi_colors",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.ansi_colors,		(long)"ANSI color codes enabled"},
{(char *)"attr_access",			cf_attr_access,	CA_GOD,		CA_DISABLED,	NULL,				(long)attraccess_nametab},
{(char *)"attr_alias",			cf_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.attr_name_htab,(long)"Attribute"},
{(char *)"attr_cmd_access",		cf_acmd_access,	CA_GOD,		CA_DISABLED,	NULL,				(long)access_nametab},
{(char *)"attr_type",			cf_attr_type,	CA_GOD,		CA_DISABLED,	NULL,				(long)attraccess_nametab},
{(char *)"autozone",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.autozone,		(long)"New objects are @chzoned to their creator's zone"},
{(char *)"bad_name",			cf_badname,	CA_GOD,		CA_DISABLED,	NULL,				0},
{(char *)"badsite_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.site_file,	MBUF_SIZE},
{(char *)"booleans_oldstyle",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.bools_oldstyle,	(long)"Dbrefs #0 and #-1 are boolean false, all other\n\t\t\t\tdbrefs are boolean true"},
{(char *)"building_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	(int *)&mudconf.building_limit,	0},
{(char *)"cache_size",			cf_int,		CA_GOD,		CA_GOD,		&mudconf.cache_size,		0},
{(char *)"cache_width",			cf_int,		CA_STATIC,	CA_GOD,		&mudconf.cache_width,		0},
{(char *)"check_interval",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.check_interval,	0},
{(char *)"check_offset",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.check_offset,		0},
{(char *)"clone_copies_cost",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.clone_copy_cost,	(long)"@clone copies object cost"},
{(char *)"command_invocation_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.cmd_invk_lim,		0},
{(char *)"command_quota_increment",	cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.cmd_quota_incr,	0},
{(char *)"command_quota_max",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.cmd_quota_max,		0},
{(char *)"command_recursion_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.cmd_nest_lim,		0},
{(char *)"concentrator_port",		cf_int,		CA_STATIC,	CA_WIZARD,	&mudconf.conc_port,		0},
{(char *)"config_access",		cf_cf_access,	CA_GOD,		CA_DISABLED,	NULL,				(long)access_nametab},
{(char *)"config_read_access",		cf_cf_access,	CA_GOD,		CA_DISABLED,	(int *)1,			(long)access_nametab},
{(char *)"conn_timeout",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.conn_timeout,		0},
{(char *)"connect_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.conn_file,	MBUF_SIZE},
{(char *)"connect_reg_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.creg_file,	MBUF_SIZE},
{(char *)"crash_database",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.crashdb,	MBUF_SIZE},
{(char *)"create_max_cost",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.createmax,		0},
{(char *)"create_min_cost",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.createmin,		0},
{(char *)"dark_actions",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.dark_actions,		(long)"Dark objects still trigger @a-actions when moving"},
{(char *)"dark_sleepers",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.dark_sleepers,		(long)"Disconnected players not shown in room contents"},
{(char *)"database_home",		cf_string,	CA_STATIC, 	CA_GOD,		(int *)&mudconf.dbhome,		MBUF_SIZE},
{(char *)"default_home",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.default_home,		NOTHING},
{(char *)"dig_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.digcost,		0},
{(char *)"divert_log",			cf_divert_log,	CA_STATIC,	CA_DISABLED,	&mudconf.log_diversion,		(long) logoptions_nametab},
{(char *)"down_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.down_file,	MBUF_SIZE},
{(char *)"down_motd_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.downmotd_msg,	GBUF_SIZE},
{(char *)"dump_interval",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.dump_interval,		0},
{(char *)"dump_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.dump_msg,	MBUF_SIZE},
{(char *)"postdump_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.postdump_msg,	MBUF_SIZE},
{(char *)"dump_offset",			cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.dump_offset,		0},
{(char *)"earn_limit",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.paylimit,		0},
{(char *)"examine_flags",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.ex_flags,		(long)"examine shows names of flags"},
{(char *)"examine_public_attrs",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.exam_public,		(long)"examine shows public attributes"},
{(char *)"exit_flags",			cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.exit_flags,	0},
{(char *)"exit_calls_move",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.exit_calls_move,	(long)"Using an exit calls the move command"},
{(char *)"exit_parent",			cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.exit_parent,		NOTHING},
{(char *)"exit_proto",			cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.exit_proto,		NOTHING},
{(char *)"exit_attr_defaults",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.exit_defobj,		NOTHING},
{(char *)"exit_quota",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.exit_quota,		0},
{(char *)"events_daily_hour",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.events_daily_hour,	0},
{(char *)"fascist_teleport",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.fascist_tport,		(long)"@teleport source restricted to control or JUMP_OK"},
{(char *)"fixed_home_message",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mudconf.fixed_home_msg,	MBUF_SIZE},
{(char *)"fixed_tel_message",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mudconf.fixed_tel_msg,	MBUF_SIZE},
{(char *)"find_money_chance",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.payfind, 		0},
{(char *)"flag_alias",			cf_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.flags_htab,	(long)"Flag"},
{(char *)"flag_access",			cf_flag_access,	CA_GOD,		CA_DISABLED,	NULL,				0},
{(char *)"flag_name",			cf_flag_name,	CA_GOD,		CA_DISABLED,	NULL,				0},
{(char *)"forbid_site",			cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.access_list,	H_FORBIDDEN},
{(char *)"fork_dump",			cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.fork_dump,		(long)"Dumps are performed using a forked process"},
{(char *)"fork_vfork",			cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.fork_vfork,		(long)"Forks are done using vfork()"},
{(char *)"forwardlist_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.fwdlist_lim,		0},
{(char *)"full_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.full_file,	MBUF_SIZE},
{(char *)"full_motd_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.fullmotd_msg,	GBUF_SIZE},
{(char *)"function_access",		cf_func_access,	CA_GOD,		CA_DISABLED,	NULL,				(long)access_nametab},
{(char *)"function_alias",		cf_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.func_htab,	(long)"Function"},
{(char *)"function_invocation_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.func_invk_lim,		0},
{(char *)"function_recursion_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.func_nest_lim,		0},
{(char *)"function_cpu_limit",		cf_int,		CA_STATIC,	CA_PUBLIC,	&mudconf.func_cpu_lim_secs,	0},
{(char *)"gdbm_database",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.gdbm,		MBUF_SIZE},
{(char *)"global_aconn_uselocks",	cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.global_aconn_uselocks,	(long)"Obey UseLocks on global @aconnect and @adisconnect"},
{(char *)"good_name",			cf_badname,	CA_GOD,		CA_DISABLED,	NULL,				1},
{(char *)"guest_basename",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mudconf.guest_basename,	PLAYER_NAME_LIMIT},
{(char *)"guest_char_num",		cf_dbref,	CA_GOD,		CA_WIZARD,	&mudconf.guest_char,		NOTHING},
{(char *)"guest_nuker",			cf_dbref,	CA_GOD,		CA_WIZARD,	&mudconf.guest_nuker,		GOD},
{(char *)"guest_password",		cf_string,	CA_GOD,		CA_GOD,		(int *)&mudconf.guest_password,	SBUF_SIZE},
{(char *)"guest_prefixes",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.guest_prefixes,	LBUF_SIZE},
{(char *)"guest_suffixes",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.guest_suffixes,	LBUF_SIZE},
{(char *)"number_guests",		cf_int,		CA_STATIC,	CA_WIZARD,	&mudconf.number_guests,		0},
{(char *)"guest_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.guest_file,	MBUF_SIZE},
{(char *)"guest_site",			cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.access_list, 	H_GUEST},
{(char *)"guest_starting_room",		cf_dbref,	CA_GOD,		CA_WIZARD,	&mudconf.guest_start_room,	NOTHING},

{(char *)"have_pueblo",			cf_const,	CA_STATIC,	CA_PUBLIC,	&mudconf.have_pueblo,		(long)"Pueblo client extensions are supported"},
{(char *)"have_zones",			cf_bool,	CA_STATIC,	CA_PUBLIC,	&mudconf.have_zones,		(long)"Multiple control via ControlLocks is permitted"},
{(char *)"helpfile",			cf_helpfile,	CA_STATIC,	CA_DISABLED,	NULL,				0},
{(char *)"hostnames",			cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.use_hostname,		(long)"DNS lookups are done on hostnames"},

#ifdef PUEBLO_SUPPORT
{(char *)"html_connect_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.htmlconn_file,	MBUF_SIZE},
{(char *)"pueblo_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.pueblo_msg,	GBUF_SIZE},
#endif

{(char *)"idle_wiz_dark",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.idle_wiz_dark,		(long)"Wizards who idle are set DARK"},
{(char *)"idle_interval",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.idle_interval,		0},
{(char *)"idle_timeout",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.idle_timeout,		0},
{(char *)"include",			cf_include,	CA_STATIC,	CA_DISABLED,	NULL,				0},
{(char *)"indent_desc",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.indent_desc,		(long)"Descriptions are indented"},
{(char *)"initial_size",		cf_int,		CA_STATIC,	CA_WIZARD,	&mudconf.init_size,		0},
{(char *)"instance_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.instance_lim,		0},
{(char *)"instant_recycle",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.instant_recycle,	(long)"@destroy instantly recycles objects set DESTROY_OK"},
{(char *)"kill_guarantee_cost",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.killguarantee,		0},
{(char *)"kill_max_cost",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.killmax,		0},
{(char *)"kill_min_cost",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.killmin,		0},
{(char *)"lag_check",			cf_const,	CA_STATIC,	CA_PUBLIC,	&mudconf.lag_check,		(long)"CPU usage warnings are enabled"},
{(char *)"lag_maximum",			cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.max_cmdsecs,		0},
{(char *)"lattr_default_oldstyle",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.lattr_oldstyle,	(long)"Empty lattr() returns blank, not #-1 NO MATCH"},
{(char *)"link_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.linkcost,		0},
{(char *)"list_access",			cf_ntab_access,	CA_GOD,		CA_DISABLED,	(int *)list_names,		(long)access_nametab},
{(char *)"local_master_rooms",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.local_masters,		(long)"Objects set ZONE act as local master rooms"},
{(char *)"lock_recursion_limit",	cf_int,		CA_WIZARD,	CA_PUBLIC,	&mudconf.lock_nest_lim,		0},
{(char *)"log",				cf_modify_bits,	CA_GOD,		CA_DISABLED,	&mudconf.log_options,		(long)logoptions_nametab},
{(char *)"log_options",			cf_modify_bits,	CA_GOD,		CA_DISABLED,	&mudconf.log_info,		(long)logdata_nametab},
{(char *)"logout_cmd_access",		cf_ntab_access,	CA_GOD,		CA_DISABLED,	(int *)logout_cmdtable,		(long)access_nametab},
{(char *)"logout_cmd_alias",		cf_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.logout_cmd_htab,(long)"Logged-out command"},
{(char *)"look_obey_terse",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.terse_look,		(long)"look obeys the TERSE flag"},
{(char *)"machine_command_cost",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.machinecost,		0},
{(char *)"master_room",			cf_dbref,	CA_GOD,		CA_WIZARD,	&mudconf.master_room,		NOTHING},
{(char *)"match_own_commands",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.match_mine,		(long)"Non-players can match $-commands on themselves"},
{(char *)"max_players",			cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.max_players,		0},
{(char *)"module",			cf_module,	CA_STATIC,	CA_WIZARD,	NULL,				0},
{(char *)"money_name_plural",		cf_string,	CA_GOD,		CA_PUBLIC,	(int *)&mudconf.many_coins,	SBUF_SIZE},
{(char *)"money_name_singular",		cf_string,	CA_GOD,		CA_PUBLIC,	(int *)&mudconf.one_coin,	SBUF_SIZE},
{(char *)"motd_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.motd_file,	MBUF_SIZE},
{(char *)"motd_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.motd_msg,	GBUF_SIZE},
{(char *)"move_match_more",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.move_match_more,	(long)"Move command checks for global and zone exits,\n\t\t\t\tresolves ambiguity"},
{(char *)"mud_name",			cf_string,	CA_GOD,		CA_PUBLIC,	(int *)&mudconf.mud_name,	SBUF_SIZE},
{(char *)"newuser_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.crea_file,	MBUF_SIZE},
{(char *)"no_ambiguous_match",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.no_ambiguous_match,	(long)"Ambiguous matches resolve to the last match"},
{(char *)"notify_recursion_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.ntfy_nest_lim,		0},
{(char *)"objeval_requires_control",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.fascist_objeval,	(long)"Control of victim required by objeval()"},
{(char *)"open_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.opencost,		0},
{(char *)"opt_frequency",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.dbopt_interval,	0},
{(char *)"output_limit",		cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.output_limit,		0},
{(char *)"page_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.pagecost,		0},
{(char *)"page_requires_equals",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.page_req_equals,	(long)"page command always requires an equals sign"},
{(char *)"paranoid_allocate",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.paranoid_alloc,	(long)"Buffer pools sanity-checked on alloc/free"},
{(char *)"parent_recursion_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.parent_nest_lim,	0},
{(char *)"paycheck",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.paycheck,		0},
{(char *)"pemit_far_players",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.pemit_players,		(long)"@pemit targets can be players in other locations"},
{(char *)"pemit_any_object",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.pemit_any,		(long)"@pemit targets can be objects in other locations"},
{(char *)"permit_site",			cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.access_list,	0},
{(char *)"player_aliases_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.max_player_aliases,	0},
{(char *)"player_flags",		cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.player_flags,	0},
{(char *)"player_listen",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.player_listen,		(long)"@listen and ^-monitors are checked on players"},
{(char *)"player_match_own_commands",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.match_mine_pl,		(long)"Players can match $-commands on themselves"},
{(char *)"player_name_spaces",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.name_spaces,		(long)"Player names can contain spaces"},
{(char *)"player_parent",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.player_parent,		NOTHING},
{(char *)"player_proto",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.player_proto,		NOTHING},
{(char *)"player_attr_defaults",	cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.player_defobj,		NOTHING},
{(char *)"player_queue_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.queuemax,		0},
{(char *)"player_quota",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.player_quota,		0},
{(char *)"player_starting_home",	cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.start_home,		NOTHING},
{(char *)"player_starting_room",	cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.start_room,		0},
{(char *)"port",			cf_int,		CA_STATIC,	CA_PUBLIC,	&mudconf.port,			0},
{(char *)"power_access",		cf_power_access,CA_GOD,		CA_DISABLED,	NULL,				0},
{(char *)"power_alias",			cf_alias,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.powers_htab,	(long)"Power"},
{(char *)"public_flags",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.pub_flags,		(long)"Flag information is public"},
{(char *)"queue_active_chunk",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.active_q_chunk,	0},
{(char *)"queue_idle_chunk",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.queue_chunk,		0},
{(char *)"quiet_look",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.quiet_look,		(long)"look shows public attributes in addition to @Desc"},
{(char *)"quiet_whisper",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.quiet_whisper,		(long)"whisper is quiet"},
{(char *)"quit_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.quit_file,	MBUF_SIZE},
{(char *)"quotas",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.quotas,		(long)"Quotas are enforced"},
{(char *)"raw_helpfile",		cf_raw_helpfile,CA_STATIC,	CA_DISABLED,	NULL,				0},
{(char *)"read_remote_desc",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.read_rem_desc,		(long)"@Desc is public, even to players not nearby"},
{(char *)"read_remote_name",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.read_rem_name,		(long)"Names are public, even to players not nearby"},
{(char *)"register_create_file",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.regf_file,	MBUF_SIZE},
{(char *)"register_site",		cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.access_list,	H_REGISTRATION},
{(char *)"require_cmds_flag",		cf_bool,	CA_GOD, 	CA_PUBLIC,	(int *)&mudconf.req_cmds_flag,	(long)"Only objects with COMMANDS flag are searched\n\t\t\t\tfor $-commands"},
{(char *)"retry_limit",			cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.retry_limit,		0},
{(char *)"robot_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.robotcost,		0},
{(char *)"robot_flags",			cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.robot_flags,	0},
{(char *)"robot_speech",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.robot_speak,		(long)"Robots can speak in locations their owners do not\n\t\t\t\tcontrol"},
{(char *)"room_flags",			cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.room_flags,	0},
{(char *)"room_parent",			cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.room_parent,		NOTHING},
{(char *)"room_proto",			cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.room_proto,		NOTHING},
{(char *)"room_attr_defaults",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.room_defobj,		NOTHING},
{(char *)"room_quota",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.room_quota,		0},
{(char *)"sacrifice_adjust",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.sacadjust,		0},
{(char *)"sacrifice_factor",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.sacfactor,		0},
{(char *)"safer_passwords",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.safer_passwords,	(long)"Passwords must satisfy minimum security standards"},
{(char *)"say_uses_comma",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.comma_say,		(long)"Say uses a grammatically-correct comma"},
{(char *)"say_uses_you",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.you_say,		(long)"Say uses You rather than the player name"},
{(char *)"search_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.searchcost,		0},
{(char *)"see_owned_dark",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.see_own_dark,		(long)"look shows DARK objects owned by you"},
{(char *)"signal_action",		cf_option,	CA_STATIC,	CA_GOD,		&mudconf.sig_action,		(long)sigactions_nametab},
{(char *)"site_chars",			cf_int,		CA_GOD,		CA_WIZARD,	&mudconf.site_chars,		MBUF_SIZE - 2},
{(char *)"space_compress",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.space_compress,	(long)"Multiple spaces are compressed to a single space"},
{(char *)"sql_database",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_db,		MBUF_SIZE},
{(char *)"sql_host",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_host,	MBUF_SIZE},
{(char *)"sql_username",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_username,	MBUF_SIZE},
{(char *)"sql_password",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_password,	MBUF_SIZE},
{(char *)"sql_reconnect",		cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.sql_reconnect,		(long)"SQL queries re-initiate dropped connections"},
{(char *)"stack_limit",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.stack_lim,		0},
{(char *)"starting_money",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.paystart,		0},
{(char *)"starting_quota",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.start_quota,		0},
{(char *)"starting_exit_quota",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.start_exit_quota,	0},
{(char *)"starting_player_quota",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.start_player_quota,	0},
{(char *)"starting_room_quota",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.start_room_quota,	0},
{(char *)"starting_thing_quota",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.start_thing_quota,	0},
{(char *)"status_file",			cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.status_file,	MBUF_SIZE},
{(char *)"stripped_flags",		cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.stripped_flags,	0},
{(char *)"structure_delimiter_string",	cf_string,	CA_GOD,		CA_PUBLIC,	(int *)&mudconf.struct_dstr,	0},
{(char *)"structure_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.struct_lim,		0},
{(char *)"suspect_site",		cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.suspect_list,	H_SUSPECT},
{(char *)"sweep_dark",			cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.sweep_dark,		(long)"@sweep works on Dark locations"},
{(char *)"switch_default_all",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.switch_df_all,		(long)"@switch default is /all, not /first"},
{(char *)"terse_shows_contents",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.terse_contents,	(long)"TERSE suppresses the contents list of a location"},
{(char *)"terse_shows_exits",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.terse_exits,		(long)"TERSE suppresses the exit list of a location"},
{(char *)"terse_shows_move_messages",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.terse_movemsg,		(long)"TERSE suppresses movement messages"},
{(char *)"thing_flags",			cf_set_flags,	CA_GOD,		CA_DISABLED,	(int *)&mudconf.thing_flags,	0},
{(char *)"thing_parent",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.thing_parent,		NOTHING},
{(char *)"thing_proto",			cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.thing_proto,		NOTHING},
{(char *)"thing_attr_defaults",		cf_dbref,	CA_GOD,		CA_PUBLIC,	&mudconf.thing_defobj,		NOTHING},
{(char *)"thing_quota",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.thing_quota,		0},
{(char *)"timeslice",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.timeslice,		0},
{(char *)"trace_output_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.trace_limit,		0},
{(char *)"trace_topdown",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.trace_topdown,		(long)"Trace output is top-down"},
{(char *)"trust_site",			cf_site,	CA_GOD,		CA_DISABLED,	(int *)&mudstate.suspect_list,	0},
{(char *)"typed_quotas",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.typed_quotas,		(long)"Quotas are enforced per object type"},
{(char *)"unowned_safe",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.safe_unowned,		(long)"Objects not owned by you are considered SAFE"},
{(char *)"user_attr_access",		cf_modify_bits,	CA_GOD,		CA_DISABLED,	&mudconf.vattr_flags,		(long)attraccess_nametab},
{(char *)"use_global_aconn",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.use_global_aconn,	(long)"Global @aconnects and @adisconnects are used"},
{(char *)"variables_limit",		cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.numvars_lim,		0},
{(char *)"visible_wizards",		cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.visible_wizzes,	(long)"DARK Wizards are hidden from WHO but not invisible"},
{(char *)"wait_cost",			cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.waitcost,		0},
{(char *)"wizard_obeys_linklock",	cf_bool,	CA_GOD,		CA_PUBLIC,	&mudconf.wiz_obey_linklock,	(long)"Check LinkLock even if player can link to anything"},
{(char *)"wizard_motd_file",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.wizmotd_file,	MBUF_SIZE},
{(char *)"wizard_motd_message",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mudconf.wizmotd_msg,	GBUF_SIZE},
{(char *)"zone_recursion_limit",	cf_int,		CA_GOD,		CA_PUBLIC,	&mudconf.zone_nest_lim,		0},
{ NULL,					NULL,		0,		0,		NULL,				0}};

/* *INDENT-ON* */

/* ---------------------------------------------------------------------------
 * cf_set: Set config parameter.
 */

static int helper_cf_set(cp, ap, player, tp)
    char *cp, *ap;
    dbref player;
    CONF *tp;
{
    int i;
    char *buff;

    if (!mudstate.standalone && !mudstate.initializing && !check_access(player, tp->flags)) {
	notify(player, NOPERM_MESSAGE);
	return (-1);
    }

    if (!mudstate.initializing) {
	buff = alloc_lbuf("cf_set");
	StringCopy(buff, ap);
    }
    i = tp->interpreter(tp->loc, ap, tp->extra, player, cp);
    if (!mudstate.initializing) {
	STARTLOG(LOG_CONFIGMODS, "CFG", "UPDAT")
	    log_name(player);
	    log_printf(" entered config directive: %s with args '%s'. Status: ", cp, strip_ansi(buff));
	    switch (i) {
		case 0:
		    log_printf("Success.");
		    break;
		case 1:
		    log_printf("Partial success.");
		    break;
		case -1:
		    log_printf("Failure.");
		    break;
		default:
		    log_printf("Strange.");
	    }
	ENDLOG
	free_lbuf(buff);
    }
    return i;
}

int cf_set(cp, ap, player)
char *cp, *ap;
dbref player;
{
	CONF *tp, *ctab;
	MODULE *mp;

	/*
	 * Search the config parameter table for the command. If we find it,
	 * call the handler to parse the argument. 
	 */

	/* Make sure that if we're standalone, the paramaters we need to
	 * load module flatfiles are loaded */
	 
	if (mudstate.standalone && strcmp(cp, "module") && 
	    strcmp(cp, "database_home")) {
		return 0;
	}

	for (tp = conftable; tp->pname; tp++) {
		if (!strcmp(tp->pname, cp)) {
		    return (helper_cf_set(cp, ap, player, tp));
		}
	}

	WALK_ALL_MODULES(mp) {
	    if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
				  CONF *)) != NULL) {
		for (tp = ctab; tp->pname; tp++) {
		    if (!strcmp(tp->pname, cp))
			return (helper_cf_set(cp, ap, player, tp));
		}
	    }
	}

	/* Config directive not found.  Complain about it. */

	if (!mudstate.standalone)
		cf_log_notfound(player, (char *)"Set", "Config directive", cp);

	return (-1);
}

/* ---------------------------------------------------------------------------
 * do_admin: Command handler to set config params at runtime 
 */

void do_admin(player, cause, extra, kw, value)
dbref player, cause;
int extra;
char *kw, *value;
{
	int i;

	i = cf_set(kw, value, player);
	if ((i >= 0) && !Quiet(player))
		notify(player, "Set.");
	return;
}

/* ---------------------------------------------------------------------------
 * cf_read: Read in config parameters from named file
 */

int cf_read(fn)
char *fn;
{
	int retval;
	char tmpfile[256], *c;
	
	mudconf.config_file = XSTRDUP(fn, "cf_read_cffile");
	mudstate.initializing = 1;
	retval = cf_include(NULL, fn, 0, 0, (char *)"init");
	mudstate.initializing = 0;

	/* Fill in missing DB file names */

	strcpy(tmpfile, mudconf.gdbm);
	if ((c = strchr(tmpfile, '.')) != NULL)
		*c = '\0';
		
	if (!*mudconf.crashdb) {
		XFREE(mudconf.crashdb, "cf_read_crashdb");
		mudconf.crashdb = XSTRDUP(tprintf("%s.CRASH", tmpfile),
					  "cf_read_crashdb");
	}
	return retval;
}

/* ---------------------------------------------------------------------------
 * list_cf_access, list_cf_read_access: List write or read access to
 * config directives.
 */

void list_cf_access(player)
dbref player;
{
	CONF *tp, *ctab;
	char *buff;
	MODULE *mp;

	buff = alloc_mbuf("list_cf_access");

	for (tp = conftable; tp->pname; tp++) {
		if (God(player) || check_access(player, tp->flags)) {
			sprintf(buff, "%s:", tp->pname);
			listset_nametab(player, access_nametab, tp->flags,
					buff, 1);
		}
	}

	WALK_ALL_MODULES(mp) {
	    if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
				  CONF *)) != NULL) {	
		for (tp = ctab; tp->pname; tp++) {
		    if (God(player) || check_access(player, tp->flags)) {
			sprintf(buff, "%s:", tp->pname);
			listset_nametab(player, access_nametab, tp->flags,
					buff, 1);
		    }
		}
	    }
	}

	free_mbuf(buff);
}

void list_cf_read_access(player)
dbref player;
{
	CONF *tp, *ctab;
	char *buff;
	MODULE *mp;

	buff = alloc_mbuf("list_cf_read_access");

	for (tp = conftable; tp->pname; tp++) {
		if (God(player) || check_access(player, tp->rperms)) {
			sprintf(buff, "%s:", tp->pname);
			listset_nametab(player, access_nametab, tp->rperms,
					buff, 1);
		}
	}

	WALK_ALL_MODULES(mp) {
	    if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
				  CONF *)) != NULL) {	
		for (tp = ctab; tp->pname; tp++) {
		    if (God(player) || check_access(player, tp->rperms)) {
			sprintf(buff, "%s:", tp->pname);
			listset_nametab(player, access_nametab, tp->rperms,
					buff, 1);
		    }
		}
	    }
	}

	free_mbuf(buff);
}

/* ---------------------------------------------------------------------------
 * cf_verify: Walk all configuration tables and validate any dbref values.
 */

#define Check_Conf_Dbref(x) \
if ((x)->interpreter == cf_dbref) { \
    if (!((((x)->extra == NOTHING) && (*((x)->loc) == NOTHING)) || \
	  (Good_obj(*((x)->loc)) && !Going(*((x)->loc))))) { \
	STARTLOG(LOG_ALWAYS, "CNF", "VRFY") \
	    log_printf("%s #%d is invalid. Reset to #%d.", \
		       (x)->pname, *((x)->loc), (x)->extra); \
	ENDLOG \
	*((x)->loc) = (dbref) (x)->extra; \
    } \
}

void cf_verify()
{
    CONF *tp, *ctab;
    MODULE *mp;

    for (tp = conftable; tp->pname; tp++) {
	Check_Conf_Dbref(tp);
    }

    WALK_ALL_MODULES(mp) {
	if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
			      CONF *)) != NULL) {
	    for (tp = ctab; tp->pname; tp++) {
		Check_Conf_Dbref(tp);
	    }
	}
    }
}

/* ---------------------------------------------------------------------------
 * cf_display: Given a config parameter by name, return its value in some
 * sane fashion.
 */

static void helper_cf_display(player, buff, bufc, tp)
    dbref player;
    char *buff, **bufc;
    CONF *tp;
{
    if (!check_access(player, tp->rperms)) {
	safe_noperm(buff, bufc);
	return;
    }
    if ((tp->interpreter == cf_bool) ||
	(tp->interpreter == cf_int) ||
	(tp->interpreter == cf_const)) {
	safe_ltos(buff, bufc, *(tp->loc));
	return;
    }
    if ((tp->interpreter == cf_string)) {
	safe_str(*((char **)tp->loc), buff, bufc);
	return;
    }
    safe_noperm(buff, bufc);
    return;
}

void cf_display(player, param_name, buff, bufc)
    dbref player;
    char *param_name;
    char *buff;
    char **bufc;
{
    CONF *tp, *ctab;
    MODULE *mp;

    for (tp = conftable; tp->pname; tp++) {
	if (!strcasecmp(tp->pname, param_name)) {
	    helper_cf_display(player, buff, bufc, tp);
	    return;
	}
    }

    WALK_ALL_MODULES(mp) {
	if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
			      CONF *)) != NULL) {	
	    for (tp = ctab; tp->pname; tp++) {
		if (!strcasecmp(tp->pname, param_name)) {
		    helper_cf_display(player, buff, bufc, tp);
		    return;
		}
	    }
	}
    }

    safe_nomatch(buff, bufc);
}

void list_options(player)
dbref player;
{
    CONF *tp, *ctab;
    MODULE *mp;

    for (tp = conftable; tp->pname; tp++) {
	if (((tp->interpreter == cf_const) ||
	     (tp->interpreter == cf_bool)) &&
	    (check_access(player, tp->rperms))) {

	    raw_notify(player, tprintf("%-25s %c %s?",
				       tp->pname,
				       (*(tp->loc) ? 'Y' : 'N'),
				       (tp->extra ? (char *)tp->extra : "")));
	}
    }

    WALK_ALL_MODULES(mp) {
	if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
			      CONF *)) != NULL) {
	    for (tp = ctab; tp->pname; tp++) {
		if (((tp->interpreter == cf_const) ||
		     (tp->interpreter == cf_bool)) &&
		    (check_access(player, tp->rperms))) {

		    raw_notify(player, tprintf("%-25s %c %s?",
					       tp->pname,
					       (*(tp->loc) ? 'Y' : 'N'),
				       (tp->extra ? (char *)tp->extra : "")));
		}
	    }
	}
    }
}
