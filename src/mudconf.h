/* mudconf.h */
/* $Id$ */

#include "copyright.h"

#ifndef __MUDCONF_H
#define __MUDCONF_H

#ifdef VMS
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#else
#include <netinet/in.h>
#endif

/* CONFDATA:	runtime configurable parameters */

typedef unsigned char Uchar;

typedef struct confdata CONFDATA;
struct confdata {
	int	cache_size;	/* Maximum size of cache */
	int	cache_width;	/* Number of cache cells */
	int	cache_names;	/* Should object names be cached separately */
	int	paylimit;	/* getting money gets hard over this much */
	int	digcost;	/* cost of @dig command */
	int	linkcost;	/* cost of @link command */
	int	opencost;	/* cost of @open command */
	int	robotcost;	/* cost of @robot command */
	int	createmin;	/* default (and minimum) cost of @create cmd */
	int	createmax;	/* max cost of @create command */
	int	quotas;		/* TRUE = have building quotas */
	int	room_quota;	/* quota needed to make a room */
	int	exit_quota;	/* quota needed to make an exit */
	int	thing_quota;	/* quota needed to make a thing */
	int	player_quota;	/* quota needed to make a robot player */
	int	sacfactor;	/* sacrifice earns (obj_cost/sfactor) + sadj */
	int	sacadjust;	/* ... */
	dbref	start_room;	/* initial location for non-Guest players */
	dbref	start_home;	/* initial HOME for players */
	dbref	default_home;	/* HOME when home is inaccessable */
	dbref	guest_start_room; /* initial location for Guests */
	int	vattr_flags;	/* Attr flags for all user-defined attrs */
	int	log_options;	/* What gets logged */
	int	log_info;	/* Info that goes into log entries */
	int	log_diversion;	/* What logs get diverted? */
	Uchar	markdata[8];	/* Masks for marking/unmarking */
	int	ntfy_nest_lim;	/* Max nesting of notifys */
	int	dbopt_interval; /* Optimize db every N dumps */
#ifndef STANDALONE
	char	indb[PBUF_SIZE];	/* database file name */
	char	outdb[PBUF_SIZE];	/* checkpoint the database to here */
	char	crashdb[PBUF_SIZE];	/* write database here on crash */
	char	gdbm[PBUF_SIZE];	/* use this gdbm file if we need one */
	char	mail_db[PBUF_SIZE];	/* name of the @mail database */
	char	comsys_db[PBUF_SIZE];	/* name of the comsys db */
	int	compress_db;	/* should we use compress */
	char	compress[PBUF_SIZE];	/* program to run to compress */
	char	uncompress[PBUF_SIZE];/* program to run to uncompress */
	char	status_file[PBUF_SIZE]; /* Where to write arg to @shutdown */
	char	mudlogname[PBUF_SIZE];	/* Name of the game log file */
        int	have_comsys;	/* Should the comsystem be active? */
        int	have_mailer;	/* Should @mail be active? */
	int	have_pueblo;	/* Is Pueblo support compiled in? */
	int	have_zones;	/* Should zones be active? */
	int	port;		/* user port */
	int	conc_port;	/* concentrator port */
	int	init_size;	/* initial db size */
	int	use_global_aconn;     /* Do we want to use global @aconn code? */
    	int	global_aconn_uselocks; /* global @aconn obeys uselocks? */
	int	have_guest;	/* Do we wish to allow a GUEST character? */
	int	guest_char;	/* player num of prototype GUEST character */
	int     guest_nuker;    /* Wiz who nukes the GUEST characters. */
	int     number_guests;  /* number of guest characters allowed */
	char    guest_prefix[SBUF_SIZE]; /* Prefix for the guest char's name */
	char	guest_file[SBUF_SIZE];	/* display if guest connects */
	char	conn_file[SBUF_SIZE];	/* display on connect if no registration */
	char	creg_file[SBUF_SIZE];	/* display on connect if registration */
	char	regf_file[SBUF_SIZE];	/* display on (failed) create if reg is on */
	char	motd_file[SBUF_SIZE];	/* display this file on login */
	char	wizmotd_file[SBUF_SIZE]; /* display this file on login to wizards */
	char	quit_file[SBUF_SIZE];	/* display on quit */
	char	down_file[SBUF_SIZE];	/* display this file if no logins */
	char	full_file[SBUF_SIZE];	/* display when max users exceeded */
	char	site_file[SBUF_SIZE];	/* display if conn from bad site */
	char	crea_file[SBUF_SIZE];	/* display this on login for new users */
	char	motd_msg[GBUF_SIZE];	/* Wizard-settable login message */
	char	wizmotd_msg[GBUF_SIZE];  /* Login message for wizards only */
	char	downmotd_msg[GBUF_SIZE];  /* Settable 'logins disabled' message */
	char	fullmotd_msg[GBUF_SIZE];  /* Settable 'Too many players' message */
	char	dump_msg[PBUF_SIZE];	/* Message displayed when @dump-ing */
	char	postdump_msg[PBUF_SIZE];  /* Message displayed after @dump-ing */
	char	fixed_home_msg[PBUF_SIZE];  /* Message displayed when going home and FIXED */
	char	fixed_tel_msg[PBUF_SIZE]; /* Message displayed when teleporting and FIXED */
	char	public_channel[SBUF_SIZE]; /* Name of public channel */
	char	guests_channel[SBUF_SIZE]; /* Name of guests channel */
	char	public_calias[SBUF_SIZE];  /* Alias of public channel */
	char	guests_calias[SBUF_SIZE];  /* Alias of guests channel */
#ifdef PUEBLO_SUPPORT
	char    pueblo_msg[GBUF_SIZE];	/* Message displayed to Pueblo clients */
	char	htmlconn_file[SBUF_SIZE];	/* display on PUEBLOCLIENT message */
#endif
	char	config_file[PBUF_SIZE]; /* name of config file, used by @restart */
	char	exec_path[MBUF_SIZE];	/* argv[0] */
	char	sql_host[MBUF_SIZE];	/* IP address of SQL database */
	char	sql_db[MBUF_SIZE];	/* Database to use */
	char	sql_username[MBUF_SIZE]; /* Username for database */
	char	sql_password[MBUF_SIZE]; /* Password for database */
	int	sql_reconnect;	/* Auto-reconnect if connection dropped? */
	int	indent_desc;	/* Newlines before and after descs? */
	int	name_spaces;	/* allow player names to have spaces */
	int	site_chars;	/* where to truncate site name */
	int	fork_dump;	/* perform dump in a forked process */
	int	fork_vfork;	/* use vfork to fork */
	int	sig_action;	/* What to do with fatal signals */
	int	paranoid_alloc;	/* Rigorous buffer integrity checks */
	int	max_players;	/* Max # of connected players */
	int	dump_interval;	/* interval between ckp dumps in seconds */
	int	check_interval;	/* interval between db check/cleans in secs */
	int	events_daily_hour; /* At what hour should @daily be executed? */
	int	dump_offset;	/* when to take first checkpoint dump */
	int	check_offset;	/* when to perform first check and clean */
	int	idle_timeout;	/* Boot off players idle this long in secs */
	int	conn_timeout;	/* Allow this long to connect before booting */
	int	idle_interval;	/* when to check for idle users */
	int	retry_limit;	/* close conn after this many bad logins */
	int	output_limit;	/* Max # chars queued for output */
	int	paycheck;	/* players earn this much each day connected */
	int	paystart;	/* new players start with this much money */
	int	start_quota;	/* Quota for new players */
	int	start_room_quota;     /* Room quota for new players */
	int	start_exit_quota;     /* Exit quota for new players */
	int	start_thing_quota;    /* Thing quota for new players */
	int	start_player_quota;   /* Player quota for new players */
	int	payfind;	/* chance to find a penny with wandering */
	int	killmin;	/* default (and minimum) cost of kill cmd */
	int	killmax;	/* max cost of kill command */
	int	killguarantee;	/* cost of kill cmd that guarantees success */
	int	pagecost;	/* cost of @page command */
	int	searchcost;	/* cost of commands that search the whole DB */
	int	waitcost;	/* cost of @wait (refunded when finishes) */
	int	building_limit;	/* max number of objects in the db */
	int	mail_expiration; /* Number of days to wait to delete mail */
	int	queuemax;	/* max commands a player may have in queue */
	int	queue_chunk;	/* # cmds to run from queue when idle */
	int	active_q_chunk;	/* # cmds to run from queue when active */
	int	machinecost;	/* One in mc+1 cmds costs 1 penny (POW2-1) */
	int	clone_copy_cost;/* Does @clone copy value? */
	int	use_hostname;	/* TRUE = use machine NAME rather than quad */
	int	typed_quotas;	/* TRUE = use quotas by type */
	int	ex_flags;	/* TRUE = show flags on examine */
	int	robot_speak;	/* TRUE = allow robots to speak in public */
	int	pub_flags;	/* TRUE = flags() works on anything */
	int	quiet_look;	/* TRUE = don't see attribs when looking */
	int	exam_public;	/* Does EXAM show public attrs by default? */
	int	read_rem_desc;	/* Can the DESCs of nonlocal objs be read? */
	int	read_rem_name;	/* Can the NAMEs of nonlocal objs be read? */
	int	sweep_dark;	/* Can you sweep dark places? */
	int	player_listen;	/* Are AxHEAR triggered on players? */
	int	quiet_whisper;	/* Can others tell when you whisper? */
	int	dark_sleepers;	/* Are sleeping players 'dark'? */
	int	see_own_dark;	/* Do you see your own dark stuff? */
	int	idle_wiz_dark;	/* Do idling wizards get set dark? */
	int	pemit_players;	/* Can you @pemit to faraway players? */
	int	pemit_any;	/* Can you @pemit to ANY remote object? */
        int	addcmd_match_blindly;   /* Does @addcommand produce a Huh?
					 * if syntax issues mean no wildcard
					 * is matched?
					 */
        int	addcmd_obey_stop;	/* Does @addcommand still multiple
					 * match on STOP objs?
					 */
	int	addcmd_obey_uselocks;	/* Does @addcommand obey uselocks? */
    	int	lattr_oldstyle; /* Bad lattr() return empty or #-1 NO MATCH? */
	int	bools_oldstyle; /* TinyMUSH 2.x and TinyMUX bools */
	int	match_mine;	/* Should you check yourself for $-commands? */
	int	match_mine_pl;	/* Should players check selves for $-cmds? */
	int	switch_df_all;	/* Should @switch match all by default? */
	int	fascist_tport;	/* Src of teleport must be owned/JUMP_OK */
	int	terse_look;	/* Does manual look obey TERSE */
	int	terse_contents;	/* Does TERSE look show exits */
	int	terse_exits;	/* Does TERSE look show obvious exits */
	int	terse_movemsg;	/* Show move msgs (SUCC/LEAVE/etc) if TERSE? */
	int	trace_topdown;	/* Is TRACE output top-down or bottom-up? */
	int	safe_unowned;	/* Are objects not owned by you safe? */
	int	trace_limit;	/* Max lines of trace output if top-down */
	int	wiz_obey_linklock;	/* Do wizards obey linklocks? */
	int	local_masters;	/* Do we check Zone rooms as local masters? */
	int	req_cmds_flag;	/* COMMANDS flag required to check $-cmds? */
	int	fmt_contents;	/* allow user-formattable Contents? */
	int	fmt_exits;	/* allow user-formattable Exits? */
	int	ansi_colors;	/* allow ANSI colors? */
	int	safer_passwords;/* enforce reasonably good password choices? */
	int	space_compress;	/* Convert multiple spaces into one space */
	int	instant_recycle;/* Do destroy_ok objects get insta-nuke? */
	int	dark_actions;	/* Trigger @a-actions even when dark? */
	int	no_ambiguous_match; /* match_result() -> last_match_result() */
	int	exit_calls_move; /* Matching an exit in the main command
				  * parser invokes the 'move' command.
				  */
	int	move_match_more; /* Exit matches in 'move' parse like the
				  * main command parser (local, global, zone;
				  * pick random on ambiguous).
				  */
	int	autozone;	/* New objs are zoned to creator's zone */
	dbref	master_room;	/* Room containing default cmds/exits/etc */
	dbref	player_proto;	/* Player prototype to clone */
	dbref	room_proto;	/* Room prototype to clone */
	dbref	exit_proto;	/* Exit prototype to clone */
	dbref	thing_proto;	/* Thing prototype to clone */
	dbref	player_parent;	/* Parent that players start with */
	dbref	room_parent;	/* Parent that rooms start with */
	dbref	exit_parent;	/* Parent that exits start with */
	dbref	thing_parent;	/* Parent that things start with */
	FLAGSET	player_flags;	/* Flags players start with */
	FLAGSET	room_flags;	/* Flags rooms start with */
	FLAGSET	exit_flags;	/* Flags exits start with */
	FLAGSET	thing_flags;	/* Flags things start with */
	FLAGSET	robot_flags;	/* Flags robots start with */
	FLAGSET stripped_flags; /* Flags stripped by @clone and @chown */
	int	abort_on_bug;	/* Dump core after logging a bug  DBG ONLY */
      	char	mud_name[SBUF_SIZE];	/* Name of the mud */
	char	one_coin[SBUF_SIZE];	/* name of one coin (ie. "penny") */
	char	many_coins[SBUF_SIZE];	/* name of many coins (ie. "pennies") */
	int	timeslice;	/* How often do we bump people's cmd quotas? */
	int	cmd_quota_max;	/* Max commands at one time */
	int	cmd_quota_incr;	/* Bump #cmds allowed by this each timeslice */
	int	lag_check;	/* Is CPU usage checking compiled in? */
	int	max_cmdsecs;	/* Threshhold for real time taken by command */
	int	control_flags;	/* Global runtime control flags */
	int	func_nest_lim;	/* Max nesting of functions */
	int	func_invk_lim;	/* Max funcs invoked by a command */
	int	lock_nest_lim;	/* Max nesting of lock evals */
	int	parent_nest_lim;/* Max levels of parents */
	int	zone_nest_lim;	/* Max nesting of zones */
	int	numvars_lim;	/* Max number of variables per object */
	int	stack_lim;	/* Max number of items on an object stack */
	int	struct_lim;	/* Max number of defined structures for obj */
	int	instance_lim;	/* Max number of struct insances for obj */
#endif	/* STANDALONE */
};

extern CONFDATA mudconf;

typedef struct site_data SITE;
struct site_data {
	struct site_data *next;		/* Next site in chain */
	struct in_addr address;		/* Host or network address */
	struct in_addr mask;		/* Mask to apply before comparing */
	int	flag;			/* Value to return on match */
};

typedef struct objlist_block OBLOCK;
struct objlist_block {
	struct objlist_block *next;
	dbref	data[(LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref)];
};

#define OBLOCK_SIZE ((LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref))

typedef struct objlist_stack OLSTK;
struct objlist_stack {
	struct objlist_stack *next;	/* Next object list in stack */
	OBLOCK	*head;		/* Head of object list */
	OBLOCK	*tail;		/* Tail of object list */
	OBLOCK	*cblock;	/* Current block for scan */
	int	count;		/* Number of objs in last obj list block */
	int	citm;		/* Current item for scan */
};

typedef struct markbuf MARKBUF;
struct markbuf {
	char	chunk[5000];
};

typedef struct alist ALIST;
struct alist {
	char	*data;
	int	len;
	struct alist *next;
};

typedef struct badname_struc BADNAME;
struct badname_struc {
	char	*name;
	struct badname_struc	*next;
};

typedef struct forward_list FWDLIST;
struct forward_list {
	int	count;
	int	*data;
};

typedef struct statedata STATEDATA;
struct statedata {
	int	record_players; /* The maximum # of player logged on */
#ifndef STANDALONE
	int	initializing;	/* are we reading config file at startup? */
	int	loading_db;	/* are we loading the db? */
	int	panicking;	/* are we in the middle of dying horribly? */
	int	restarting;	/* Are we restarting? */
	int	dumping;	/* Are we dumping? */
	int	logging;	/* Are we in the middle of logging? */
	int	epoch;		/* Generation number for dumps */
	int	generation;	/* DB global generation number */
	int	mudlognum;	/* Number of logfile */
    	int	helpfiles;	/* Number of external indexed helpfiles */
	int	hfiletab_size;	/* Size of the table storing path pointers */
    	char	**hfiletab;	/* Array of path pointers */
    	HASHTAB *hfile_hashes;	/* Pointer to an array of index hashtables */
	dbref	curr_enactor;	/* Who initiated the current command */
	dbref	curr_player;	/* Who is running the current command */
	char	*curr_cmd;	/* The current command */
      	int	alarm_triggered;/* Has periodic alarm signal occurred? */
	time_t	now;		/* What time is it now? */
	time_t	dump_counter;	/* Countdown to next db dump */
	time_t	check_counter;	/* Countdown to next db check */
	time_t	idle_counter;	/* Countdown to next idle check */
	time_t	mstats_counter;	/* Countdown to next mstats snapshot */
	time_t  events_counter; /* Countdown to next events check */
	int	events_flag;	/* Flags for check_events */
	int	shutdown_flag;	/* Should interface be shut down? */
	char	version[PBUF_SIZE];	/* MUSH version string */
	char	short_ver[64];  /* Short version number (for INFO) */
	time_t	start_time;	/* When was MUSH started */
	time_t	restart_time;	/* When did we last restart? */
	int	reboot_nums;	/* How many times have we restarted? */
    	time_t	cpu_count_from; /* When did we last reset CPU counters? */
	char	buffer[LBUF_SIZE * 2];	/* A buffer for holding temp stuff */
	char	*debug_cmd;	/* The command we are executing (if any) */
	char	doing_hdr[DOING_LEN]; /* Doing column header in WHO display */
	SITE	*access_list;	/* Access states for sites */
	SITE	*suspect_list;	/* Sites that are suspect */
	HASHTAB	command_htab;	/* Commands hashtable */
	HASHTAB	logout_cmd_htab;/* Logged-out commands hashtable (WHO, etc) */
	HASHTAB func_htab;	/* Functions hashtable */
	HASHTAB ufunc_htab;	/* Local functions hashtable */
	HASHTAB powers_htab;    /* Powers hashtable */
	HASHTAB flags_htab;	/* Flags hashtable */
	HASHTAB	attr_name_htab;	/* Attribute names hashtable */
	HASHTAB vattr_name_htab;/* User attribute names hashtable */
	HASHTAB player_htab;	/* Player name->number hashtable */
	NHSHTAB	desc_htab;	/* Socket descriptor hashtable */
	NHSHTAB	fwdlist_htab;	/* Room forwardlists */
	NHSHTAB objstack_htab;	/* Object stacks */
	NHSHTAB	parent_htab;	/* Parent $-command exclusion */
#ifdef PARSE_TREES
	NHSHTAB tree_htab;	/* Parse trees for evaluation */
#endif
	HASHTAB vars_htab;	/* Persistent variables hashtable */
	HASHTAB structs_htab;	/* Structure hashtable */
	HASHTAB cdefs_htab;	/* Components hashtable */
	HASHTAB instance_htab;	/* Instances hashtable */
	HASHTAB instdata_htab;	/* Structure data hashtable */
#ifdef USE_COMSYS
	HASHTAB comsys_htab;	/* Channels hashtable */
	HASHTAB calias_htab;	/* Channel aliases */
	NHSHTAB comlist_htab;	/* Player channel lists */
#endif
#ifdef USE_MAIL
	NHSHTAB mail_htab;	/* Mail players hashtable */
#endif
	int	max_structs;
	int	max_cdefs;
	int	max_instance;
	int	max_instdata;
	int	max_stacks;
	int	max_vars;
	int	attr_next;	/* Next attr to alloc when freelist is empty */
	BQUE	*qfirst;	/* Head of player queue */
	BQUE	*qlast;		/* Tail of player queue */
	BQUE	*qlfirst;	/* Head of object queue */
	BQUE	*qllast;	/* Tail of object queue */
	BQUE	*qwait;		/* Head of wait queue */
	BQUE	*qsemfirst;	/* Head of semaphore queue */
	BQUE	*qsemlast;	/* Tail of semaphore queue */
	BADNAME	*badname_head;	/* List of disallowed names */
	int	mstat_ixrss[2];	/* Summed shared size */
	int	mstat_idrss[2];	/* Summed private data size */
	int	mstat_isrss[2];	/* Summed private stack size */
	int	mstat_secs[2];	/* Time of samples */
	int	mstat_curr;	/* Which sample is latest */
	ALIST	iter_alist;	/* Attribute list for iterations */
	char	*mod_alist;	/* Attribute list for modifying */
	int	mod_size;	/* Length of modified buffer */
	dbref	mod_al_id;	/* Where did mod_alist come from? */
	OLSTK	*olist;		/* Stack of object lists for nested searches */
	dbref	freelist;	/* Head of object freelist */
	int	min_size;	/* Minimum db size (from file header) */
	int	db_top;		/* Number of items in the db */
	int	db_size;	/* Allocated size of db structure */
	int	mail_freelist;  /* The next free mail number */
	int	mail_db_top;    /* Like db_top */
	int	mail_db_size;	/* Like db_size */
	MENT	*mail_list;     /* The mail database */
	int	*guest_free;	/* Table to keep track of free guests */
	MARKBUF	*markbits;	/* temp storage for marking/unmarking */
	int	in_loop;	/* In a loop() statement? */
	char	*loop_token[MAX_ITER_NESTING];	/* Value of ## */
	int	loop_number[MAX_ITER_NESTING];	/* Value of #@ */
	int	in_switch;	/* In a switch() statement? */
	char	*switch_token;	/* Value of #$ */
	int	func_nest_lev;	/* Current nesting of functions */
	int	func_invk_ctr;	/* Functions invoked so far by this command */
	int	ntfy_nest_lev;	/* Current nesting of notifys */
	int	lock_nest_lev;	/* Current nesting of lock evals */
	char	*global_regs[MAX_GLOBAL_REGS];	/* Global registers */
	int	glob_reg_len[MAX_GLOBAL_REGS];	/* Length of strs */
	int	zone_nest_num;  /* Global current zone nest position */
	int	inpipe;		/* Boolean flag for command piping */
	char	*pout;		/* The output of the pipe used in %| */
	char	*poutnew;	/* The output being build by the current command */
	char	*poutbufc; 	/* Buffer position for poutnew */
	dbref	poutobj;	/* Object doing the piping */
	int	sql_socket;	/* Socket fd for SQL database connection */
#else
	int	logging;	/* Are we in the middle of logging? */
	char	buffer[256];	/* A buffer for holding temp stuff */
	int	attr_next;	/* Next attr to alloc when freelist is empty */
	ALIST	iter_alist;	/* Attribute list for iterations */
	char	*mod_alist;	/* Attribute list for modifying */
	int	mod_size;	/* Length of modified buffer */
	dbref	mod_al_id;	/* Where did mod_alist come from? */
	int	min_size;	/* Minimum db size (from file header) */
	int	db_top;		/* Number of items in the db */
	int	db_size;	/* Allocated size of db structure */
	dbref	freelist;	/* Head of object freelist */
	MARKBUF	*markbits;	/* temp storage for marking/unmarking */
	HASHTAB vattr_name_htab;/* User attribute names hashtable */
#endif	/* STANDALONE */
};

extern STATEDATA mudstate;

/* Configuration parameter handler definition */

#define CF_HAND(proc)	int proc (vp, str, extra, player, cmd) \
			int	*vp; \
			char	*str, *cmd; \
			long	extra; \
			dbref	player;
/* This is for the DEC Alpha, which can't cast a pointer to an int. */
#define CF_AHAND(proc)	int proc (vp, str, extra, player, cmd) \
			long	**vp; \
			char	*str, *cmd; \
			long	extra; \
			dbref	player;
			
#define CF_HDCL(proc)	int FDECL(proc, (long *, char *, long, dbref, char *))

/* Global flags */

/* Game control flags in mudconf.control_flags */

#define	CF_LOGIN	0x0001		/* Allow nonwiz logins to the MUSH */
#define CF_BUILD	0x0002		/* Allow building commands */
#define CF_INTERP	0x0004		/* Allow object triggering */
#define CF_CHECKPOINT	0x0008		/* Perform auto-checkpointing */
#define	CF_DBCHECK	0x0010		/* Periodically check/clean the DB */
#define CF_IDLECHECK	0x0020		/* Periodically check for idle users */
/* empty		0x0040 */
/* empty		0x0080 */
#define CF_DEQUEUE	0x0100		/* Remove entries from the queue */
#define CF_GODMONITOR   0x0200  /* Display commands to the God. */
#define CF_EVENTCHECK   0x0400		/* Allow events checking */
/* Host information codes */

#define H_REGISTRATION	0x0001	/* Registration ALWAYS on */
#define H_FORBIDDEN	0x0002	/* Reject all connects */
#define H_SUSPECT	0x0004	/* Notify wizards of connects/disconnects */
#define H_GUEST         0x0008  /* Don't permit guests from here */

/* Event flags, for noting when an event has taken place */

#define ET_DAILY	0x00000001	/* Daily taken place? */

/* Logging options */

#define LOG_ALLCOMMANDS	0x00000001	/* Log all commands */
#define LOG_ACCOUNTING	0x00000002	/* Write accounting info on logout */
#define LOG_BADCOMMANDS	0x00000004	/* Log bad commands */
#define LOG_BUGS	0x00000008	/* Log program bugs found */
#define LOG_DBSAVES	0x00000010	/* Log database dumps */
#define LOG_CONFIGMODS	0x00000020	/* Log changes to configuration */
#define LOG_PCREATES	0x00000040	/* Log character creations */
#define LOG_KILLS	0x00000080	/* Log KILLs */
#define LOG_LOGIN	0x00000100	/* Log logins and logouts */
#define LOG_NET		0x00000200	/* Log net connects and disconnects */
#define LOG_SECURITY	0x00000400	/* Log security-related events */
#define LOG_SHOUTS	0x00000800	/* Log shouts */
#define LOG_STARTUP	0x00001000	/* Log nonfatal errors in startup */
#define LOG_WIZARD	0x00002000	/* Log dangerous things */
#define LOG_ALLOCATE	0x00004000	/* Log alloc/free from buffer pools */
#define LOG_PROBLEMS	0x00008000	/* Log runtime problems */
#define LOG_KBCOMMANDS	0x00010000	/* Log keyboard commands */
#define LOG_SUSPECTCMDS	0x00020000	/* Log SUSPECT player keyboard cmds */
#define LOG_TIMEUSE	0x00040000	/* Log CPU time usage */
#define LOG_LOCAL	0x00080000	/* Log user stuff via @log */
#define LOG_ALWAYS	0x80000000	/* Always log it */

#define LOGOPT_FLAGS		0x01	/* Report flags on object */
#define LOGOPT_LOC		0x02	/* Report loc of obj when requested */
#define LOGOPT_OWNER		0x04	/* Report owner of obj if not obj */
#define LOGOPT_TIMESTAMP	0x08	/* Timestamp log entries */

#endif /* __MUDCONF_H */
