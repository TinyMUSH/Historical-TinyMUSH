/* command.c - command parser and support routines */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "functions.h"
#include "externs.h"
#include "match.h"
#include "attrs.h"
#include "flags.h"
#include "powers.h"
#include "alloc.h"
#include "vattr.h"
#include "mail.h"
#include "db_sql.h"

extern void FDECL(list_cf_access, (dbref));
extern void FDECL(list_siteinfo, (dbref));
extern void FDECL(logged_out, (dbref, dbref, int, char *));
extern void NDECL(boot_slave);
extern void NDECL(vattr_clean_db);

#define CACHING "object"

/* Take care of all the assorted problems associated with getrusage(). */

#ifdef hpux
#define HAVE_GETRUSAGE 1
#include <sys/syscall.h>
#define getrusage(x,p)   syscall(SYS_GETRUSAGE,x,p)
#endif

#ifdef _SEQUENT_
#define HAVE_GET_PROCESS_STATS 1
#include <sys/procstats.h>
#endif

/* ---------------------------------------------------------------------------
 * Hook macros.
 *
 * We never want to call hooks in the case of @addcommand'd commands
 * (both for efficiency reasons and the fact that we might NOT match an
 * @addcommand even if we've been told there is one), but we leave this
 * to the hook-adder to prevent.
 */

#define CALL_PRE_HOOK(x,a,na) \
if (((x)->pre_hook != NULL) && !((x)->callseq & CS_ADDED)) { \
    process_hook((x)->pre_hook, (x)->callseq & CS_PRESERVE, \
                 player, cause, (a), (na)); \
}

#define CALL_POST_HOOK(x,a,na) \
if (((x)->post_hook != NULL) && !((x)->callseq & CS_ADDED)) { \
    process_hook((x)->post_hook, (x)->callseq & CS_PRESERVE, \
                 player, cause, (a), (na)); \
}

/* ---------------------------------------------------------------------------
 * Switch tables for the various commands.
 */

#define	SW_MULTIPLE	0x80000000	/* This sw may be spec'd w/others */
#define	SW_GOT_UNIQUE	0x40000000	/* Already have a unique option */
#define SW_NOEVAL       0x20000000      /* Don't parse args before calling
					 * handler
					 */
					/* (typically via a switch alias) */
/* *INDENT-OFF* */

NAMETAB attrib_sw[] = {
{(char *)"access",	1,	CA_GOD,		ATTRIB_ACCESS},
{(char *)"delete",	1,	CA_GOD,		ATTRIB_DELETE},
{(char *)"info",	1,	CA_WIZARD,	ATTRIB_INFO},
{(char *)"rename",	1,	CA_GOD,		ATTRIB_RENAME},
{ NULL,			0,	0,		0}};

NAMETAB boot_sw[] = {
{(char *)"port",	1,	CA_WIZARD,	BOOT_PORT|SW_MULTIPLE},
{(char *)"quiet",	1,	CA_WIZARD,	BOOT_QUIET|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

#ifdef USE_COMSYS
NAMETAB cemit_sw[] = {
{(char *)"noheader",	1,	CA_PUBLIC,	CEMIT_NOHEADER},
{ NULL,			0,	0,		0}};
#endif

NAMETAB clone_sw[] = {
{(char *)"cost",	1,	CA_PUBLIC,	CLONE_SET_COST},
{(char *)"inherit",	3,	CA_PUBLIC,	CLONE_INHERIT|SW_MULTIPLE},
{(char *)"inventory",	3,	CA_PUBLIC,	CLONE_INVENTORY},
{(char *)"location",	1,	CA_PUBLIC,	CLONE_LOCATION},
{(char *)"parent",	2,	CA_PUBLIC,	CLONE_PARENT|SW_MULTIPLE},
{(char *)"preserve",	2,	CA_WIZARD,	CLONE_PRESERVE|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

#ifdef USE_COMSYS
NAMETAB clist_sw[] = {
{(char *)"full",        0,      CA_PUBLIC,      CLIST_FULL},
{ NULL,                 0,      0,              0}};

NAMETAB cset_sw[] = {
{(char *)"public",      2,      CA_PUBLIC,      CSET_PUBLIC},
{(char *)"private",     2,      CA_PUBLIC,      CSET_PRIVATE},
{(char *)"loud",        2,      CA_PUBLIC,      CSET_LOUD},
{(char *)"quiet",       1,      CA_PUBLIC,      CSET_QUIET},
{(char *)"mute",        1,      CA_PUBLIC,      CSET_QUIET},
{(char *)"list",        2,      CA_PUBLIC,      CSET_LIST},
{(char *)"object",      2,      CA_PUBLIC,      CSET_OBJECT},
{ NULL,                 0,      0,              0}}; 
#endif

NAMETAB decomp_sw[] = {
{(char *)"dbref",	1,	CA_PUBLIC,	DECOMP_DBREF},
{(char *)"pretty",	1,	CA_PUBLIC,	DECOMP_PRETTY},
{ NULL,			0,	0,		0}};

NAMETAB destroy_sw[] = {
{(char *)"override",	8,	CA_PUBLIC,	DEST_OVERRIDE},
{ NULL,			0,	0,		0}};

NAMETAB dig_sw[] = {
{(char *)"teleport",	1,	CA_PUBLIC,	DIG_TELEPORT},
{ NULL,			0,	0,		0}};

NAMETAB doing_sw[] = {
{(char *)"header",	1,	CA_PUBLIC,	DOING_HEADER},
{(char *)"message",	1,	CA_PUBLIC,	DOING_MESSAGE},
{(char *)"poll",	1,	CA_PUBLIC,	DOING_POLL},
{ NULL,			0,	0,		0}};

NAMETAB dolist_sw[] = {
{(char *)"delimit",     1,      CA_PUBLIC,      DOLIST_DELIMIT},
{(char *)"space",       1,      CA_PUBLIC,      DOLIST_SPACE},
{(char *)"notify",	1,	CA_PUBLIC,	DOLIST_NOTIFY | SW_MULTIPLE },
{ NULL,                 0,      0,              0,}};

NAMETAB	drop_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	DROP_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB dump_sw[] = {
{(char *)"structure",	1,	CA_WIZARD,	DUMP_STRUCT|SW_MULTIPLE},
{(char *)"text",	1,	CA_WIZARD,	DUMP_TEXT|SW_MULTIPLE},
{(char *)"flatfile",	1,	CA_WIZARD,	DUMP_FLATFILE|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB emit_sw[] = {
{(char *)"here",	1,	CA_PUBLIC,	SAY_HERE|SW_MULTIPLE},
{(char *)"room",	1,	CA_PUBLIC,	SAY_ROOM|SW_MULTIPLE},
#ifdef PUEBLO_SUPPORT
{(char *)"html",	1,      CA_PUBLIC,      SAY_HTML|SW_MULTIPLE},
#endif
{ NULL,			0,	0,		0}};

NAMETAB	enter_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	MOVE_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB examine_sw[] = {
{(char *)"brief",	1,	CA_PUBLIC,	EXAM_BRIEF},
{(char *)"debug",	1,	CA_WIZARD,	EXAM_DEBUG},
{(char *)"full",	1,	CA_PUBLIC,	EXAM_LONG},
{(char *)"parent",	1,	CA_PUBLIC,	EXAM_PARENT},
{(char *)"pretty",	2,	CA_PUBLIC,	EXAM_PRETTY},
{ NULL,			0,	0,		0}};

NAMETAB femit_sw[] = {
{(char *)"here",	1,	CA_PUBLIC,	PEMIT_HERE|SW_MULTIPLE},
{(char *)"room",	1,	CA_PUBLIC,	PEMIT_ROOM|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB fixdb_sw[] = {
/* {(char *)"add_pname",1,	CA_GOD,		FIXDB_ADD_PN}, */
{(char *)"contents",	1,	CA_GOD,		FIXDB_CON},
{(char *)"exits",	1,	CA_GOD,		FIXDB_EXITS},
{(char *)"location",	1,	CA_GOD,		FIXDB_LOC},
{(char *)"next",	1,	CA_GOD,		FIXDB_NEXT},
{(char *)"owner",	1,	CA_GOD,		FIXDB_OWNER},
{(char *)"pennies",	1,	CA_GOD,		FIXDB_PENNIES},
{(char *)"rename",	1,	CA_GOD,		FIXDB_NAME},
/* {(char *)"rm_pname",	1,	CA_GOD,		FIXDB_DEL_PN}, */
{ NULL,			0,	0,		0}};

NAMETAB fpose_sw[] = {
{(char *)"default",	1,	CA_PUBLIC,	0},
{(char *)"nospace",	1,	CA_PUBLIC,	SAY_NOSPACE},
{ NULL,			0,	0,		0}};

NAMETAB	function_sw[] = {
{(char *)"privileged",	3,	CA_WIZARD,	FN_PRIV|SW_MULTIPLE},
{(char *)"preserve",	3,	CA_WIZARD,	FN_PRES|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB	get_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	GET_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB	give_sw[] = {
{(char *)"quiet",	1,	CA_WIZARD,	GIVE_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB	goto_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	MOVE_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB halt_sw[] = {
{(char *)"all",		1,	CA_PUBLIC,	HALT_ALL},
{ NULL,			0,	0,		0}};

NAMETAB hook_sw[] = {
{(char *)"before",	1,	CA_GOD,		HOOK_BEFORE},
{(char *)"after",	1,	CA_GOD,		HOOK_AFTER},
{(char *)"preserve",	1,	CA_GOD,		HOOK_PRESERVE},
{(char *)"nopreserve",	1,	CA_GOD,		HOOK_NOPRESERVE},
{ NULL,			0,	0,		0}};

NAMETAB	leave_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	MOVE_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB listmotd_sw[] = {
{(char *)"brief",	1,	CA_WIZARD,	MOTD_BRIEF},
{ NULL,			0,	0,		0}};

NAMETAB lock_sw[] = {
{(char *)"chownlock",	2,	CA_PUBLIC,	A_LCHOWN},
{(char *)"controllock",	2,	CA_PUBLIC,	A_LCONTROL},
{(char *)"defaultlock",	1,	CA_PUBLIC,	A_LOCK},
{(char *)"droplock",	1,	CA_PUBLIC,	A_LDROP},
{(char *)"enterlock",	1,	CA_PUBLIC,	A_LENTER},
{(char *)"givelock",	1,	CA_PUBLIC,	A_LGIVE},
{(char *)"leavelock",	2,	CA_PUBLIC,	A_LLEAVE},
{(char *)"linklock",	2,	CA_PUBLIC,	A_LLINK},
{(char *)"pagelock",	3,	CA_PUBLIC,	A_LPAGE},
{(char *)"parentlock",	3,	CA_PUBLIC,	A_LPARENT},
{(char *)"receivelock",	1,	CA_PUBLIC,	A_LRECEIVE},
{(char *)"teloutlock",	2,	CA_PUBLIC,	A_LTELOUT},
{(char *)"tportlock",	2,	CA_PUBLIC,	A_LTPORT},
{(char *)"uselock",	1,	CA_PUBLIC,	A_LUSE},
{(char *)"userlock",	4,	CA_PUBLIC,	A_LUSER},
{(char *)"speechlock",	1,	CA_PUBLIC,	A_LSPEECH},
{ NULL,			0,	0,		0}};

NAMETAB look_sw[] = {
{(char *)"outside",     1,      CA_PUBLIC,      LOOK_OUTSIDE},
{ NULL,                 0,      0,              0}};

#ifdef USE_MAIL
NAMETAB mail_sw[] = {
{(char *)"stats",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_STATS},
{(char *)"dstats",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_DSTATS},
{(char *)"fstats",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_FSTATS},
{(char *)"debug",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_DEBUG},
{(char *)"nuke",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_NUKE},
{(char *)"folder",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_FOLDER},
{(char *)"list",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_LIST},
{(char *)"read",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_READ},
{(char *)"clear",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_CLEAR},
{(char *)"unclear",     1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_UNCLEAR},
{(char *)"purge",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_PURGE},
{(char *)"file",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_FILE},
{(char *)"tag",         1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_TAG},
{(char *)"untag",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_UNTAG},
{(char *)"fwd",         2,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_FORWARD},
{(char *)"forward",     2,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_FORWARD},
{(char *)"send",        0,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_SEND},
{(char *)"edit",        2,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_EDIT},
{(char *)"urgent",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_URGENT},
{(char *)"alias",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_ALIAS},
{(char *)"alist",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_ALIST},
{(char *)"proof",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MAIL_PROOF},
{(char *)"abort",	0,	CA_NO_SLAVE|CA_NO_GUEST,      MAIL_ABORT},
{(char *)"quick",	0,	CA_NO_SLAVE|CA_NO_GUEST,      MAIL_QUICK},
{(char *)"review",	2,	CA_NO_SLAVE|CA_NO_GUEST,      MAIL_REVIEW},
{(char *)"retract",	2,	CA_NO_SLAVE|CA_NO_GUEST,      MAIL_RETRACT},
{(char *)"cc",		2,	CA_NO_SLAVE|CA_NO_GUEST,	MAIL_CC},
{(char *)"safe",	2,	CA_NO_SLAVE|CA_NO_GUEST,	MAIL_SAFE},
{ NULL,                 0,      0,              0}};

NAMETAB malias_sw[] = {
{(char *)"desc",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_DESC},
{(char *)"chown",       1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_CHOWN},
{(char *)"add",         1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_ADD},
{(char *)"remove",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_REMOVE},
{(char *)"delete",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_DELETE},
{(char *)"rename",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_RENAME},
{(char *)"list",        1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_LIST},
{(char *)"status",      1,      CA_NO_SLAVE|CA_NO_GUEST,      MALIAS_STATUS},
{ NULL,                 0,      0,              0}};
#endif

NAMETAB mark_sw[] = {
{(char *)"set",		1,	CA_PUBLIC,	MARK_SET},
{(char *)"clear",	1,	CA_PUBLIC,	MARK_CLEAR},
{ NULL,			0,	0,		0}};

NAMETAB markall_sw[] = {
{(char *)"set",		1,	CA_PUBLIC,	MARK_SET},
{(char *)"clear",	1,	CA_PUBLIC,	MARK_CLEAR},
{ NULL,			0,	0,		0}};

NAMETAB motd_sw[] = {
{(char *)"brief",	1,	CA_WIZARD,	MOTD_BRIEF|SW_MULTIPLE},
{(char *)"connect",	1,	CA_WIZARD,	MOTD_ALL},
{(char *)"down",	1,	CA_WIZARD,	MOTD_DOWN},
{(char *)"full",	1,	CA_WIZARD,	MOTD_FULL},
{(char *)"list",	1,	CA_PUBLIC,	MOTD_LIST},
{(char *)"wizard",	1,	CA_WIZARD,	MOTD_WIZ},
{ NULL,			0,	0,		0}};

NAMETAB notify_sw[] = {
{(char *)"all",		1,	CA_PUBLIC,	NFY_NFYALL},
{(char *)"first",	1,	CA_PUBLIC,	NFY_NFY},
{ NULL,			0,	0,		0}};

NAMETAB open_sw[] = {
{(char *)"inventory",	1,	CA_PUBLIC,	OPEN_INVENTORY},
{(char *)"location",	1,	CA_PUBLIC,	OPEN_LOCATION},
{ NULL,			0,	0,		0}};

NAMETAB pemit_sw[] = {
{(char *)"contents",	1,	CA_PUBLIC,	PEMIT_CONTENTS|SW_MULTIPLE},
{(char *)"object",	1,	CA_PUBLIC,	0},
{(char *)"silent",	1,	CA_PUBLIC,	0},
{(char *)"list",        1,      CA_PUBLIC,      PEMIT_LIST|SW_MULTIPLE},
{(char *)"noeval",	1,	CA_PUBLIC,	SW_NOEVAL | SW_MULTIPLE},
#ifdef PUEBLO_SUPPORT
{(char *)"html",	1,      CA_PUBLIC,      PEMIT_HTML|SW_MULTIPLE},
#endif
{ NULL,			0,	0,		0}};

NAMETAB pose_sw[] = {
{(char *)"default",	1,	CA_PUBLIC,	0},
{(char *)"nospace",	1,	CA_PUBLIC,	SAY_NOSPACE},
{ NULL,			0,	0,		0}};

NAMETAB ps_sw[] = {
{(char *)"all",		1,	CA_PUBLIC,	PS_ALL|SW_MULTIPLE},
{(char *)"brief",	1,	CA_PUBLIC,	PS_BRIEF},
{(char *)"long",	1,	CA_PUBLIC,	PS_LONG},
{(char *)"summary",	1,	CA_PUBLIC,	PS_SUMM},
{ NULL,			0,	0,		0}};

NAMETAB quota_sw[] = {
{(char *)"all",		1,	CA_GOD,		QUOTA_ALL|SW_MULTIPLE},
{(char *)"fix",		1,	CA_WIZARD,	QUOTA_FIX},
{(char *)"remaining",	1,	CA_WIZARD,	QUOTA_REM|SW_MULTIPLE},
{(char *)"set",		1,	CA_WIZARD,	QUOTA_SET},
{(char *)"total",	1,	CA_WIZARD,	QUOTA_TOT|SW_MULTIPLE},
{(char *)"room",	1,	CA_WIZARD,	QUOTA_ROOM | SW_MULTIPLE},
{(char *)"exit",	1,	CA_WIZARD,	QUOTA_EXIT | SW_MULTIPLE},
{(char *)"thing",	1,	CA_WIZARD,	QUOTA_THING | SW_MULTIPLE},
{(char *)"player",	1,	CA_WIZARD,	QUOTA_PLAYER | SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB	set_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	SET_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB shutdown_sw[] = {
{(char *)"abort",	1,	CA_WIZARD,	SHUTDN_COREDUMP},
{ NULL,			0,	0,		0}};

NAMETAB stats_sw[] = {
{(char *)"all",		1,	CA_PUBLIC,	STAT_ALL},
{(char *)"me",		1,	CA_PUBLIC,	STAT_ME},
{(char *)"player",	1,	CA_PUBLIC,	STAT_PLAYER},
{ NULL,			0,	0,		0}};

NAMETAB sweep_sw[] = {
{(char *)"commands",	3,	CA_PUBLIC,	SWEEP_COMMANDS|SW_MULTIPLE},
{(char *)"connected",	3,	CA_PUBLIC,	SWEEP_CONNECT|SW_MULTIPLE},
{(char *)"exits",	1,	CA_PUBLIC,	SWEEP_EXITS|SW_MULTIPLE},
{(char *)"here",	1,	CA_PUBLIC,	SWEEP_HERE|SW_MULTIPLE},
{(char *)"inventory",	1,	CA_PUBLIC,	SWEEP_ME|SW_MULTIPLE},
{(char *)"listeners",	1,	CA_PUBLIC,	SWEEP_LISTEN|SW_MULTIPLE},
{(char *)"players",	1,	CA_PUBLIC,	SWEEP_PLAYER|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB switch_sw[] = {
{(char *)"all",		1,	CA_PUBLIC,	SWITCH_ANY},
{(char *)"default",	1,	CA_PUBLIC,	SWITCH_DEFAULT},
{(char *)"first",	1,	CA_PUBLIC,	SWITCH_ONE},
{ NULL,			0,	0,		0}};

NAMETAB teleport_sw[] = {
{(char *)"loud",	1,	CA_PUBLIC,	TELEPORT_DEFAULT},
{(char *)"quiet",	1,	CA_PUBLIC,	TELEPORT_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB timecheck_sw[] = {
{(char *) "log",	1,	CA_WIZARD,	TIMECHK_LOG | SW_MULTIPLE},
{(char *) "reset",	1,	CA_WIZARD,	TIMECHK_RESET | SW_MULTIPLE},
{(char *) "screen",	1,	 CA_WIZARD,	TIMECHK_SCREEN | SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB toad_sw[] = {
{(char *)"no_chown",	1,	CA_WIZARD,	TOAD_NO_CHOWN|SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB	trig_sw[] = {
{(char *)"quiet",	1,	CA_PUBLIC,	TRIG_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB wall_sw[] = {
{(char *)"emit",	1,	CA_PUBLIC,	SAY_WALLEMIT},
{(char *)"no_prefix",	1,	CA_PUBLIC,	SAY_NOTAG|SW_MULTIPLE},
{(char *)"pose",	1,	CA_PUBLIC,	SAY_WALLPOSE},
{(char *)"wizard",	1,	CA_PUBLIC,	SAY_WIZSHOUT|SW_MULTIPLE},
{(char *)"admin",	1,	CA_ADMIN,	SAY_ADMINSHOUT},
{ NULL,			0,	0,		0}};

NAMETAB warp_sw[] = {
{(char *)"check",	1,	CA_WIZARD,	TWARP_CLEAN|SW_MULTIPLE},
{(char *)"dump",	1,	CA_WIZARD,	TWARP_DUMP|SW_MULTIPLE},
{(char *)"idle",	1,	CA_WIZARD,	TWARP_IDLE|SW_MULTIPLE},
{(char *)"queue",	1,	CA_WIZARD,	TWARP_QUEUE|SW_MULTIPLE},
{(char *)"events",	1,	CA_WIZARD,	TWARP_EVENTS|SW_MULTIPLE},
{ NULL,			0,	0,		0}};


/* ---------------------------------------------------------------------------
 * Command table: Definitions for builtin commands, used to build the command
 * hash table.
 *
 * Format:  Name		Switches	Permissions Needed
 *	Key (if any)	Calling Seq			Handler
 */

CMDENT command_table[] = {
{(char *)"@@",			NULL,		0,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_comment},
{(char *)"@addcommand",		NULL,		CA_GOD,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_addcommand},
{(char *)"@admin",		NULL,		CA_WIZARD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_admin},
{(char *)"@alias",		NULL,		CA_NO_GUEST|CA_NO_SLAVE,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_alias},
{(char *)"@apply_marked",	NULL,		CA_WIZARD|CA_GBL_INTERP,
	0,		CS_ONE_ARG|CS_CMDARG|CS_NOINTERP|CS_STRIP_AROUND,
	NULL,			NULL,		do_apply_marked},
{(char *)"@attribute",		attrib_sw,	CA_WIZARD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_attribute},
{(char *)"@boot",		boot_sw,	CA_NO_GUEST|CA_NO_SLAVE,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_boot},
{(char *)"@cboot",              NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_TWO_ARG,          
	NULL,			NULL,		do_chboot},
{(char *)"@ccharge",            NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        1,               CS_TWO_ARG,          
	NULL,			NULL,		do_editchannel},
{(char *)"@cchown",             NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_TWO_ARG,          
	NULL,			NULL,		do_editchannel},
{(char *)"@ccreate",            NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_ONE_ARG,          
	NULL,			NULL,		do_createchannel},
{(char *)"@cdestroy",           NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_ONE_ARG,          
	NULL,			NULL,		do_destroychannel},
{(char *)"@cemit",		cemit_sw,		CA_NO_SLAVE|CA_NO_GUEST,
	0,		 CS_TWO_ARG,		
	NULL,			NULL,		do_cemit},
{(char *)"@chown",		NULL,
	CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
	CHOWN_ONE,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_chown},
{(char *)"@chownall",		NULL,		CA_WIZARD|CA_GBL_BUILD,
	CHOWN_ALL,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_chownall},
{(char *)"@chzone",             NULL,           
        CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
        0,              CS_TWO_ARG|CS_INTERP, 
	NULL,			NULL,		do_chzone},
{(char *)"@clone",		clone_sw,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_CONTENTS|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_clone},
{(char *)"@clist",              clist_sw,       CA_NO_SLAVE,
        0,              CS_NO_ARGS,           
	NULL,			NULL,		do_chanlist},
{(char *)"@coflags",            NULL,           CA_NO_SLAVE,
        4,              CS_TWO_ARG,           
	NULL,			NULL,		do_editchannel},
{(char *)"@cpattr",             NULL,           
         CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
         0,             CS_TWO_ARG|CS_ARGV,   
	NULL,			NULL,		do_cpattr},
{(char *)"@cpflags",            NULL,           CA_NO_SLAVE,
        3,              CS_TWO_ARG,           
	NULL,			NULL,		do_editchannel},
{(char *)"@create",		NULL,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_CONTENTS|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_create},
{(char *)"@cset",               cset_sw,        CA_NO_SLAVE,
        0,              CS_TWO_ARG|CS_INTERP, 
	NULL,			NULL,		do_chopen},
{(char *)"@cut",		NULL,		CA_WIZARD|CA_LOCATION,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_cut},
{(char *)"@cwho",               NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,			NULL,		do_channelwho},
{(char *)"@dbck",		NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_dbck},
{(char *)"@dbclean",		NULL,		CA_GOD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_dbclean},
{(char *)"@decompile",		decomp_sw,		0,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_decomp},
{(char *)"@delcommand",		NULL,		CA_GOD,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_delcommand},
{(char *)"@destroy",		destroy_sw,
	CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
	DEST_ONE,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_destroy},
/*{(char *)"@destroyall",	NULL,		CA_WIZARD|CA_GBL_BUILD,
	DEST_ALL,	CS_ONE_ARG,		
	NULL,			NULL,		do_destroy}, */
{(char *)"@dig",		dig_sw,
	CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
	0,		CS_TWO_ARG|CS_ARGV|CS_INTERP,
	NULL,			NULL,		do_dig},
{(char *)"@disable",		NULL,		CA_WIZARD,
	GLOB_DISABLE,	CS_ONE_ARG,		
	NULL,			NULL,		do_global},
{(char *)"@doing",		doing_sw,	CA_PUBLIC,
	0,		CS_ONE_ARG,		
	NULL,			NULL,		do_doing},
{(char *)"@dolist",		dolist_sw,		CA_GBL_INTERP,
	0,		CS_TWO_ARG|CS_CMDARG|CS_NOINTERP|CS_STRIP_AROUND,
                                              
	NULL,			NULL,		do_dolist},
{(char *)"@drain",		NULL,
	CA_GBL_INTERP|CA_NO_SLAVE|CA_NO_GUEST,
	NFY_DRAIN,	CS_TWO_ARG,		
	NULL,			NULL,		do_notify},
{(char *)"@dump",		dump_sw,	CA_WIZARD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_dump},
{(char *)"@edit",		NULL,		CA_NO_SLAVE|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_ARGV|CS_STRIP_AROUND,
						
	NULL,			NULL,		do_edit},
{(char *)"@emit",		emit_sw,
	CA_LOCATION|CA_NO_GUEST|CA_NO_SLAVE,
	SAY_EMIT,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_say},
{(char *)"@enable",		NULL,		CA_WIZARD,
	GLOB_ENABLE,	CS_ONE_ARG,		
	NULL,			NULL,		do_global},
{(char *)"@entrances",		NULL,		CA_NO_GUEST,
	0,		CS_ONE_ARG|CS_INTERP,
	NULL,			NULL,		do_entrances},
{(char *)"@eval",		NULL,		CA_NO_SLAVE,
	0,		CS_ONE_ARG | CS_INTERP,
	NULL,			NULL,		do_eval},
{(char *)"@femit",		femit_sw,
	CA_LOCATION|CA_NO_GUEST|CA_NO_SLAVE,
	PEMIT_FEMIT,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"@find",		NULL,		0,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_find},
{(char *)"@fixdb",		fixdb_sw,	CA_GOD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_fixdb},
{(char *)"@force",		NULL,
	CA_NO_SLAVE|CA_GBL_INTERP|CA_NO_GUEST,
	FRC_COMMAND,	CS_TWO_ARG|CS_INTERP|CS_CMDARG,
	NULL,			NULL,		do_force},
{(char *)"@fpose",		fpose_sw,	CA_LOCATION|CA_NO_SLAVE,
	PEMIT_FPOSE,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"@fsay",		NULL,		CA_LOCATION|CA_NO_SLAVE,
	PEMIT_FSAY,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"@freelist",		NULL,		CA_WIZARD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_freelist},
{(char *)"@function",		function_sw,	CA_GOD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_function},
{(char *)"@halt",		halt_sw,	CA_NO_SLAVE,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_halt},
{(char *)"@hook",		hook_sw,	CA_GOD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_hook},
{(char *)"@kick",		NULL,		CA_WIZARD,
	QUEUE_KICK,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_queue},
{(char *)"@last",		NULL,		CA_NO_GUEST,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_last},
{(char *)"@link",		NULL,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_link},
{(char *)"@list",		NULL,		0,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_list},
{(char *)"@listcommands",		NULL,		CA_GOD,
	0,		CS_ONE_ARG,		
	NULL,			NULL,		do_listcommands},
{(char *)"@list_file",		NULL,		CA_WIZARD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_list_file},
{(char *)"@listmotd",		listmotd_sw,	0,
	MOTD_LIST,	CS_ONE_ARG,		
	NULL,			NULL,		do_motd},
{(char *)"@lock",		lock_sw,	CA_NO_SLAVE,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_lock},
{(char *)"@logrotate",		NULL,		CA_GOD,
	0,		CS_NO_ARGS,
	NULL,			NULL,		do_logrotate},
#ifdef USE_MAIL
{(char *)"@mail",               mail_sw,           CA_NO_SLAVE|CA_NO_GUEST,
        0,              CS_TWO_ARG|CS_INTERP,
	NULL,			NULL,		do_mail},
{(char *)"@malias",             malias_sw,         CA_NO_SLAVE|CA_NO_GUEST,
        0,              CS_TWO_ARG|CS_INTERP,
	NULL,			NULL,		do_malias},
#endif
{(char *)"@mark",		mark_sw,	CA_WIZARD,
	SRCH_MARK,	CS_ONE_ARG|CS_NOINTERP,	
	NULL,			NULL,		do_search},
{(char *)"@mark_all",		markall_sw,	CA_WIZARD,
	MARK_SET,	CS_NO_ARGS,		
	NULL,			NULL,		do_markall},
{(char *)"@motd",		motd_sw,	CA_WIZARD,
	0,		CS_ONE_ARG,		
	NULL,			NULL,		do_motd},
{(char *)"@mvattr",		NULL,
	CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
	0,		CS_TWO_ARG|CS_ARGV,	
	NULL,			NULL,		do_mvattr},
{(char *)"@name",		NULL,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_name},
{(char *)"@newpassword",	NULL,		CA_WIZARD,
	PASS_ANY,	CS_TWO_ARG,		
	NULL,			NULL,		do_newpassword},
{(char *)"@notify",		notify_sw,
	CA_GBL_INTERP|CA_NO_SLAVE|CA_NO_GUEST,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_notify},
{(char *)"@oemit",		NULL,
	CA_LOCATION|CA_NO_GUEST|CA_NO_SLAVE,
	PEMIT_OEMIT,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"@open",		open_sw,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_ARGV|CS_INTERP,
	NULL,			NULL,		do_open},
{(char *)"@parent",		NULL,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_parent},
{(char *)"@password",		NULL,		CA_NO_GUEST,
	PASS_MINE,	CS_TWO_ARG,		
	NULL,			NULL,		do_password},
{(char *)"@pcreate",		NULL,		CA_WIZARD|CA_GBL_BUILD,
	PCRE_PLAYER,	CS_TWO_ARG,		
	NULL,			NULL,		do_pcreate},
{(char *)"@pemit",		pemit_sw,	CA_NO_GUEST|CA_NO_SLAVE,
	PEMIT_PEMIT,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"@npemit",		pemit_sw,	CA_NO_GUEST|CA_NO_SLAVE,
	PEMIT_PEMIT,	CS_TWO_ARG|CS_UNPARSE|CS_NOSQUISH,	
	NULL,			NULL,		do_pemit},
{(char *)"@poor",		NULL,		CA_GOD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_poor},
{(char *)"@power",		NULL,		0,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_power},
{(char *)"@program",		NULL,		CA_PUBLIC,
	0,		CS_TWO_ARG|CS_INTERP,		
	NULL,			NULL,		do_prog},
{(char *)"@ps",			ps_sw,		0,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_ps},
{(char *)"@quota",		quota_sw,	0,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_quota},
{(char *)"@quitprogram",	NULL,		CA_PUBLIC,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_quitprog},
{(char *)"@readcache",		NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_readcache},
{(char *)"@restart",		NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_restart},
{(char *)"@robot",		NULL,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST|CA_PLAYER,
	PCRE_ROBOT,	CS_TWO_ARG,		
	NULL,			NULL,		do_pcreate},
{(char *)"@search",		NULL,		0,
	SRCH_SEARCH,	CS_ONE_ARG|CS_NOINTERP,	
	NULL,			NULL,		do_search},
{(char *)"@set",		set_sw,
	CA_NO_SLAVE|CA_GBL_BUILD|CA_NO_GUEST,
	0,		CS_TWO_ARG,		
	NULL,			NULL,		do_set},
{(char *)"@shutdown",		NULL,		CA_WIZARD,
	0,		CS_ONE_ARG,		
	NULL,			NULL,		do_shutdown},
{(char *)"@sql",		NULL,		CA_SQL_OK,
	0,		CS_ONE_ARG,
	NULL,			NULL,		do_sql},
{(char *)"@sqlconnect",		NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,
	NULL,			NULL,		do_sql_connect},
{(char *)"@sqldisconnect",	NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,
	NULL,			NULL,		sql_shutdown},
{(char *)"@stats",		stats_sw,	0,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_stats},
{(char *)"@startslave",		NULL,		CA_WIZARD,
	0,		CS_NO_ARGS,			
	NULL,			NULL,		boot_slave},
{(char *)"@sweep",		sweep_sw,	0,
	0,		CS_ONE_ARG,		
	NULL,			NULL,		do_sweep},
{(char *)"@switch",		switch_sw,	CA_GBL_INTERP,
	0,		CS_TWO_ARG|CS_ARGV|CS_CMDARG|CS_NOINTERP|CS_STRIP_AROUND,
						
	NULL,			NULL,		do_switch},
{(char *)"@teleport",		teleport_sw,	CA_NO_GUEST,
	TELEPORT_DEFAULT, CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_teleport},
{(char *)"@timecheck",		timecheck_sw,	CA_WIZARD,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_timecheck},
{(char *)"@timewarp",		warp_sw,	CA_WIZARD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_timewarp},
{(char *)"@toad",		toad_sw,	CA_WIZARD,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_toad},
{(char *)"@trigger",		trig_sw,	CA_GBL_INTERP,
	0,		CS_TWO_ARG|CS_ARGV,	
	NULL,			NULL,		do_trigger},
{(char *)"@unlink",		NULL,		CA_NO_SLAVE|CA_GBL_BUILD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_unlink},
{(char *)"@unlock",		lock_sw,	CA_NO_SLAVE,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_unlock},
{(char *)"@verb",		NULL,		CA_GBL_INTERP|CA_NO_SLAVE,
	0,		CS_TWO_ARG|CS_ARGV|CS_INTERP|CS_STRIP_AROUND,
						
	NULL,			NULL,		do_verb},
{(char *)"@wait",		NULL,		CA_GBL_INTERP,
	0,		CS_TWO_ARG|CS_CMDARG|CS_NOINTERP|CS_STRIP_AROUND,
						
	NULL,			NULL,		do_wait},
{(char *)"@wall",		wall_sw,	CA_PUBLIC,
	SAY_SHOUT,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_say},
{(char *)"@wipe",		NULL,
	CA_NO_SLAVE|CA_NO_GUEST|CA_GBL_BUILD,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_wipe},
{(char *)"addcom",              NULL,           CA_NO_SLAVE,
        0,              CS_TWO_ARG,           
	NULL,			NULL,		do_addcom},
{(char *)"allcom",              NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,			NULL,		do_allcom},
{(char *)"comlist",             NULL,           CA_NO_SLAVE,
        0,              CS_NO_ARGS,           
	NULL,			NULL,		do_comlist},
{(char *)"comtitle",            NULL,           CA_NO_SLAVE,
        0,              CS_TWO_ARG,          
	NULL,			NULL,		do_comtitle},
{(char *)"clearcom",            NULL,           CA_NO_SLAVE,
        0,              CS_NO_ARGS,           
	NULL,			NULL,		do_clearcom},
{(char *)"delcom",              NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,			NULL,		do_delcom},
{(char *)"drop",		drop_sw,
	CA_NO_SLAVE|CA_CONTENTS|CA_LOCATION|CA_NO_GUEST,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_drop},
{(char *)"enter",		enter_sw,	CA_LOCATION,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_enter},
{(char *)"examine",		examine_sw,	0,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_examine},
{(char *)"get",			get_sw,		CA_LOCATION|CA_NO_GUEST,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_get},
{(char *)"give",		give_sw,	CA_LOCATION|CA_NO_GUEST,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_give},
{(char *)"goto",		goto_sw,	CA_LOCATION,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_move},
{(char *)"inventory",		NULL,		0,
	LOOK_INVENTORY,	CS_NO_ARGS,		
	NULL,			NULL,		do_inventory},
{(char *)"kill",		NULL,		CA_NO_GUEST|CA_NO_SLAVE,
	KILL_KILL,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_kill},
{(char *)"leave",		leave_sw,		CA_LOCATION,
	0,		CS_NO_ARGS|CS_INTERP,	
	NULL,			NULL,		do_leave},
{(char *)"look",		look_sw,		CA_LOCATION,
	LOOK_LOOK,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_look},
{(char *)"page",		NULL,		CA_NO_SLAVE,
	0,		CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_page},
{(char *)"pose",		pose_sw,	CA_LOCATION|CA_NO_SLAVE,
	SAY_POSE,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_say},
{(char *)"say",			NULL,		CA_LOCATION|CA_NO_SLAVE,
	SAY_SAY,	CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_say},
{(char *)"score",		NULL,		0,
	LOOK_SCORE,	CS_NO_ARGS,		
	NULL,			NULL,		do_score},
{(char *)"slay",		NULL,		CA_WIZARD,
	KILL_SLAY,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_kill},
{(char *)"think",               NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,			NULL,		do_think},
{(char *)"use",			NULL,		CA_NO_SLAVE|CA_GBL_INTERP,
	0,		CS_ONE_ARG|CS_INTERP,	
	NULL,			NULL,		do_use},
{(char *)"version",		NULL,		0,
	0,		CS_NO_ARGS,		
	NULL,			NULL,		do_version},
{(char *)"whisper",		NULL,		CA_LOCATION|CA_NO_SLAVE,
	PEMIT_WHISPER,	CS_TWO_ARG|CS_INTERP,	
	NULL,			NULL,		do_pemit},
{(char *)"doing",		NULL,		CA_PUBLIC,
	CMD_DOING,	CS_ONE_ARG,		
	NULL,			NULL,		logged_out},
{(char *)"quit",		NULL,		CA_PUBLIC,
	CMD_QUIT,	CS_NO_ARGS,		
	NULL,			NULL,		logged_out},
{(char *)"logout",		NULL,		CA_PUBLIC,
	CMD_LOGOUT,	CS_NO_ARGS,		
	NULL,			NULL,		logged_out},
{(char *)"who",			NULL,		CA_PUBLIC,
	CMD_WHO,	CS_ONE_ARG,		
	NULL,			NULL,		logged_out},
{(char *)"session",		NULL,		CA_PUBLIC,
	CMD_SESSION,	CS_ONE_ARG,		
	NULL,			NULL,		logged_out},
{(char *)"outputprefix",	NULL,		CA_PUBLIC,
	CMD_PREFIX,	CS_ONE_ARG,		
	NULL,			NULL,		logged_out},
{(char *)"outputsuffix",	NULL,		CA_PUBLIC,
	CMD_SUFFIX,	CS_ONE_ARG,		
	NULL,			NULL,		logged_out},
{(char *)"puebloclient",	NULL,           CA_PUBLIC,
      CMD_PUEBLOCLIENT,CS_ONE_ARG,              
	NULL,			NULL,		logged_out},
{(char *)"\\",			NULL,
	CA_NO_GUEST|CA_LOCATION|CF_DARK|CA_NO_SLAVE,
	SAY_PREFIX,	CS_ONE_ARG|CS_INTERP|CS_LEADIN,
	NULL,			NULL,		do_say},
{(char *)"#",			NULL,
	CA_NO_SLAVE|CA_GBL_INTERP|CF_DARK,
	0,		CS_ONE_ARG|CS_INTERP|CS_CMDARG|CS_LEADIN,
	NULL,			NULL,		do_force_prefixed},
{(char *)":",			NULL,
	CA_LOCATION|CF_DARK|CA_NO_SLAVE,
	SAY_PREFIX,	CS_ONE_ARG|CS_INTERP|CS_LEADIN,
	NULL,			NULL,		do_say},
{(char *)";",			NULL,
	CA_LOCATION|CF_DARK|CA_NO_SLAVE,
	SAY_PREFIX,	CS_ONE_ARG|CS_INTERP|CS_LEADIN,
	NULL,			NULL,		do_say},
{(char *)"\"",			NULL,
	CA_LOCATION|CF_DARK|CA_NO_SLAVE,
	SAY_PREFIX,	CS_ONE_ARG|CS_INTERP|CS_LEADIN,
	NULL,			NULL,		do_say},
{(char *)"&",			NULL,
	CA_NO_GUEST|CA_NO_SLAVE|CF_DARK,
	0,		CS_TWO_ARG|CS_LEADIN,	
	NULL,			NULL,		do_setvattr},
{(char *)"-",			NULL,
	CA_NO_GUEST|CA_NO_SLAVE|CF_DARK,
	0,		CS_ONE_ARG|CS_LEADIN,	
	NULL,			NULL,		do_postpend},
{(char *)"~",			NULL,
	CA_NO_GUEST|CA_NO_SLAVE|CF_DARK,
	0,		CS_ONE_ARG|CS_LEADIN,	
	NULL,			NULL,		do_prepend},
{(char *)NULL,			NULL,		0,
	0,		0,				
	NULL,		NULL,			NULL}};

/* *INDENT-ON* */

CMDENT *prefix_cmds[256];

CMDENT *goto_cmdp, *enter_cmdp, *leave_cmdp;

void NDECL(init_cmdtab)
{
	CMDENT *cp;
	ATTR *ap;
	char *p, *q;
	char *cbuff;

	hashinit(&mudstate.command_htab, 250 * HASH_FACTOR);

	/* Load attribute-setting commands */

	cbuff = alloc_sbuf("init_cmdtab");
	for (ap = attr; ap->name; ap++) {
		if ((ap->flags & AF_NOCMD) == 0) {
			p = cbuff;
			*p++ = '@';
			for (q = (char *)ap->name; *q; p++, q++)
				*p = ToLower(*q);
			*p = '\0';
			cp = (CMDENT *) XMALLOC(sizeof(CMDENT), "init_cmdtab");
			cp->cmdname = (char *)strsave(cbuff);
			cp->perms = CA_NO_GUEST | CA_NO_SLAVE;
			cp->switches = NULL;
			if (ap->flags & (AF_WIZARD | AF_MDARK)) {
				cp->perms |= CA_WIZARD;
			}
			cp->extra = ap->number;
			cp->callseq = CS_TWO_ARG;
			cp->pre_hook = NULL;
			cp->post_hook = NULL;
			cp->info.handler = do_setattr;
			if (hashadd(cp->cmdname, (int *)cp, &mudstate.command_htab)) {
				XFREE(cp->cmdname, "init_cmdtab.2");
				XFREE(cp, "init_cmdtab.3");
			}
		}
	}
	free_sbuf(cbuff);

	/* Load the builtin commands */	

	for (cp = command_table; cp->cmdname; cp++)
		hashadd(cp->cmdname, (int *)cp, &mudstate.command_htab);

	set_prefix_cmds();
	
	goto_cmdp = (CMDENT *) hashfind("goto", &mudstate.command_htab);
	enter_cmdp = (CMDENT *) hashfind("enter", &mudstate.command_htab);
	leave_cmdp = (CMDENT *) hashfind("leave", &mudstate.command_htab);
}

void set_prefix_cmds()
{
int i;

	/* Load the command prefix table.  Note - these commands can never
	 * be typed in by a user because commands are lowercased
	 * before the hash table is checked. The names are
	 * abbreviated to minimise name checking time. 
	 */

	for (i = 0; i < A_USER_START; i++)
		prefix_cmds[i] = NULL;
	prefix_cmds['"'] = (CMDENT *) hashfind((char *)"\"",
					       &mudstate.command_htab);
	prefix_cmds[':'] = (CMDENT *) hashfind((char *)":",
					       &mudstate.command_htab);
	prefix_cmds[';'] = (CMDENT *) hashfind((char *)";",
					       &mudstate.command_htab);
	prefix_cmds['\\'] = (CMDENT *) hashfind((char *)"\\",
						&mudstate.command_htab);
	prefix_cmds['#'] = (CMDENT *) hashfind((char *)"#",
					       &mudstate.command_htab);
	prefix_cmds['&'] = (CMDENT *) hashfind((char *)"&",
					       &mudstate.command_htab);
#ifdef USE_MAIL
	/* Note that doing it this way means that you'll later run into
	 * problems if you enable the mailer without a @restart. However,
	 * not doing it this way breaks all the softcoded mailers.
	 */
	if (mudconf.have_mailer) {
	    prefix_cmds['-'] = (CMDENT *) hashfind((char *)"-",
						   &mudstate.command_htab);
	    prefix_cmds['~'] = (CMDENT *) hashfind((char *)"~",
						   &mudstate.command_htab);
	}
#endif
}

/* ---------------------------------------------------------------------------
 * check_access: Check if player has access to function.  
 */

int check_access(player, mask)
dbref player;
int mask;
{
	int succ, fail;

	if (mask & CA_DISABLED)
		return 0;
	if (mask & CA_STATIC)
		return 0;
	if (God(player) || mudstate.initializing)
		return 1;

	succ = fail = 0;
	if (mask & CA_GOD)
		fail++;
	if (mask & CA_WIZARD) {
		if (Wizard(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_ADMIN)) {
		if (WizRoy(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_BUILDER)) {
		if (Builder(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_STAFF)) {
		if (Staff(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_HEAD)) {
		if (Head(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_IMMORTAL)) {
		if (Immortal(player))
			succ++;
		else
			fail++;
	}
	if (mask & CA_SQL_OK) {
	    if (Can_Use_SQL(player))
		succ++;
	    else
		fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER0)) {
		if (H_Marker0(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER1)) {
		if (H_Marker1(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER2)) {
		if (H_Marker2(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER3)) {
		if (H_Marker3(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER4)) {
		if (H_Marker4(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER5)) {
		if (H_Marker5(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER6)) {
		if (H_Marker6(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER7)) {
		if (H_Marker7(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER8)) {
		if (H_Marker8(player))
			succ++;
		else
			fail++;
	}
	if ((succ == 0) && (mask & CA_MARKER9)) {
		if (H_Marker9(player))
			succ++;
		else
			fail++;
	}
	if (succ > 0)
		fail = 0;
	if (fail > 0)
		return 0;

	/* Check for forbidden flags. */

	if (!Wizard(player) &&
	    (((mask & CA_NO_HAVEN) && Player_haven(player)) ||
	     ((mask & CA_NO_ROBOT) && Robot(player)) ||
	     ((mask & CA_NO_SLAVE) && Slave(player)) ||
	     ((mask & CA_NO_SUSPECT) && Suspect(player)) ||
	     ((mask & CA_NO_GUEST) && Guest(player)))) {
		return 0;
	}
	return 1;
}

/* ---------------------------------------------------------------------------
 * process_hook: Evaluate a hook function.
 */

static void process_hook(hp, save_globs, player, cause, cargs, ncargs)
    HOOKENT *hp;
    int save_globs;
    dbref player, cause;
    char *cargs[];
    int ncargs;
{
    char *buf, *bp;
    char *tstr, *str;
    dbref aowner;
    int aflags;
    char *preserve[MAX_GLOBAL_REGS];

    /* We know we have a non-null hook. We want to evaluate the obj/attr
     * pair of that hook. We consider the enactor to be the player who
     * executed the command that caused this hook to be called.
     */

    tstr = atr_get(hp->thing, hp->atr, &aowner, &aflags);
    str = tstr;

    if (!tstr)
	return;

    if (save_globs) {
	save_global_regs("process_hook", preserve);
    }

    buf = bp = alloc_lbuf("process_hook");
    exec(buf, &bp, 0, hp->thing, player, EV_EVAL | EV_FIGNORE | EV_TOP,
	 &str, cargs, ncargs);
    *bp = '\0';
    free_lbuf(buf);

    if (save_globs) {
	restore_global_regs("process_hook", preserve);
    }

    free_lbuf(tstr);
}

/* ---------------------------------------------------------------------------
 * process_cmdent: Perform indicated command with passed args.
 */

void process_cmdent(cmdp, switchp, player, cause, interactive, arg,
		    unp_command, cargs, ncargs)
CMDENT *cmdp;
char *switchp, *arg, *unp_command, *cargs[];
dbref player, cause;
int interactive, ncargs;
{
	char *buf1, *buf2, tchar, *bp, *str, *buff, *s, *j, *new;
	char *args[MAX_ARG];
	int nargs, i, fail, interp, key, xkey, aflags;
	int hasswitch = 0;
	int cmd_matches = 0;
	dbref aowner;
	char *aargs[10];
	ADDENT *add;
	
#define	Protect(x) (cmdp->perms & x)

	/* Perform object type checks. */

	fail = 0;
	if (Protect(CA_LOCATION) && !Has_location(player))
		fail++;
	if (Protect(CA_CONTENTS) && !Has_contents(player))
		fail++;
	if (Protect(CA_PLAYER) && (Typeof(player) != TYPE_PLAYER))
		fail++;
	if (fail > 0) {
		notify(player, "Command incompatible with invoker type.");
		return;
	}
	/* Check if we have permission to execute the command */
    
	if (!check_access(player, cmdp->perms)) {
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Check global flags */

	if ((!Builder(player)) && Protect(CA_GBL_BUILD) && !(mudconf.control_flags & CF_BUILD)) {
		notify(player, "Sorry, building is not allowed now.");
		return;
	}
	if (Protect(CA_GBL_INTERP) && !(mudconf.control_flags & CF_INTERP)) {
		notify(player,
		     "Sorry, queueing and triggering are not allowed now.");
		return;
	}
	key = cmdp->extra & ~SW_MULTIPLE;
	if (key & SW_GOT_UNIQUE) {
		i = 1;
		key = key & ~SW_GOT_UNIQUE;
	} else {
		i = 0;
	}

	/* Check command switches.  Note that there may be more than one, 
	 * and that we OR all of them together along with the extra value
	 * from the command table to produce the key value in the handler call. 
	 */

	if (switchp && cmdp->switches) {
		do {
			buf1 = (char *)index(switchp, '/');
			if (buf1)
				*buf1++ = '\0';
			xkey = search_nametab(player, cmdp->switches,
					      switchp);
			if (xkey == -1) {
				notify(player,
				       tprintf("Unrecognized switch '%s' for command '%s'.",
					       switchp, cmdp->cmdname));
				return;
			} else if (xkey == -2) {
				notify(player, NOPERM_MESSAGE);
				return;
			} else if (!(xkey & SW_MULTIPLE)) {
				if (i == 1) {
					notify(player,
					"Illegal combination of switches.");
					return;
				}
				i = 1;
			} else {
				xkey &= ~SW_MULTIPLE;
			}
			key |= xkey;
			switchp = buf1;
			hasswitch = 1;
		} while (buf1);
	} else if (switchp && !(cmdp->callseq & CS_ADDED)) {
		notify(player,
		       tprintf("Command %s does not take switches.",
			       cmdp->cmdname));
		return;
	}

	/* At this point we're guaranteed we're going to execute something.
	 * Let's check to see if we have a pre-command hook.
	 */

	CALL_PRE_HOOK(cmdp, cargs, ncargs);

	/* If the command normally has interpreted args, but the user
	 * specified, /noeval, just do EV_STRIP.
	 *
	 * If the command is interpreted, or we're interactive (and
	 * the command isn't specified CS_NOINTERP), eval the args.
	 * 
	 * The others are obvious.
	 */
	if ((cmdp->callseq & CS_INTERP) && (key & SW_NOEVAL)) {
		interp = EV_STRIP;
		key &= ~SW_NOEVAL;	/* Remove SW_NOEVAL from 'key' */
	}
	else if ((cmdp->callseq & CS_INTERP) ||
		!(interactive || (cmdp->callseq & CS_NOINTERP)))
		interp = EV_EVAL | EV_STRIP;
	else if (cmdp->callseq & CS_STRIP)
		interp = EV_STRIP;
	else if (cmdp->callseq & CS_STRIP_AROUND)
		interp = EV_STRIP_AROUND;
	else
		interp = 0;

	switch (cmdp->callseq & CS_NARG_MASK) {
	case CS_NO_ARGS:	/* <cmd>   (no args) */
		(*(cmdp->info.handler)) (player, cause, key);
		break;
	case CS_ONE_ARG:	/* <cmd> <arg> */

		/* If an unparsed command, just give it to the handler */

		if (cmdp->callseq & CS_UNPARSE) {
			(*(cmdp->info.handler)) (player, unp_command);
			break;
		}
		/* Interpret if necessary, but not twice for CS_ADDED */

		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED)) {
			buf1 = bp = alloc_lbuf("process_cmdent");
			str = arg;
			exec(buf1, &bp, 0, player, cause, interp | EV_FCHECK | EV_TOP,
			     &str, cargs, ncargs);
			*bp = '\0';
		} else
			buf1 = parse_to(&arg, '\0', interp | EV_TOP);

		/* Call the correct handler */

		if (cmdp->callseq & CS_CMDARG) {
			(*(cmdp->info.handler)) (player, cause, key, buf1,
					    cargs, ncargs);
		} else {
		    if (cmdp->callseq & CS_ADDED) {

			/* Construct the matching buffer. */

			/* In the case of a single-letter prefix, we want
			 * to just skip past that first letter. Otherwise
			 * we want to go past the first word.
			 */
			if (!(cmdp->callseq & CS_LEADIN)) {
			    for (j = unp_command; *j && (*j != ' '); j++) ;
			} else {
			    j = unp_command; j++;
			}
			new = alloc_lbuf("process_cmdent.soft");
			bp = new;
			if (!*j) {
			    /* No args */
			    if (!(cmdp->callseq & CS_LEADIN)) {
				safe_str(cmdp->cmdname, new, &bp);
			    } else {
				safe_str(unp_command, new, &bp);
			    }
			    if (switchp) {
				safe_chr('/', new, &bp);
				safe_str(switchp, new, &bp);
			    }
			    *bp = '\0';
			} else {
			    if (!(cmdp->callseq & CS_LEADIN))
				j++;
			    safe_str(cmdp->cmdname, new, &bp);
			    if (switchp) {
				safe_chr('/', new, &bp);
				safe_str(switchp, new, &bp);
			    }
			    if (!(cmdp->callseq & CS_LEADIN))
				safe_chr(' ', new, &bp);
			    safe_str(j, new, &bp);
			    *bp = '\0';
			} 

			/* Now search against the attributes. */

			for (add = (ADDENT *)cmdp->info.added;
			     add != NULL; add = add->next) {
			    buff = atr_get(add->thing,
					   add->atr, &aowner, &aflags);
			    /* Skip the '$' character, and the next */
			    for (s = buff + 2; *s && (*s != ':'); s++) ;
			    if (!*s)
				break;
			    *s++ = '\0';
			    
			    if (wild(buff + 1, new, aargs, 10)) {
				if (!mudconf.addcmd_obey_uselocks ||
				    could_doit(player, add->thing, A_LUSE)) {
				    wait_que(add->thing, player,
					     0, NOTHING, 0, s, aargs, 10,
					     mudstate.global_regs);
				    for (i = 0; i < 10; i++) {
					if (aargs[i])
					    free_lbuf(aargs[i]);
				    }
				    cmd_matches++;
				}
			    }
			    free_lbuf(buff);
			    if (cmd_matches && mudconf.addcmd_obey_stop &&
				Stop_Match(add->thing)) {
				break;
			    }
			}

			if (!cmd_matches && !mudconf.addcmd_match_blindly) {
			    /* The command the player typed didn't match
			     * any of the wildcard patterns we have for
			     * that addcommand. We should raise an error.
			     * We DO NOT go back into trying to match
			     * other stuff -- this is a 'Huh?' situation.
			     */
			    notify(player, "Huh?  (Type \"help\" for help.)");
			    STARTLOG(LOG_BADCOMMANDS, "CMD", "BAD")
				log_name_and_loc(player);
			        log_text((char *) " entered: ");
			        log_text(new);
			    ENDLOG
			}

			free_lbuf(new);

		    } else 
			(*(cmdp->info.handler)) (player, cause, key, buf1);
		}

		/* Free the buffer if one was allocated */

		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED))
			free_lbuf(buf1);

		break;
	case CS_TWO_ARG:	/* <cmd> <arg1> = <arg2> */

		/* Interpret ARG1 */

		buf2 = parse_to(&arg, '=', EV_STRIP_TS);

		/* Handle when no '=' was specified */

		if (!arg || (arg && !*arg)) {
			arg = &tchar;
			*arg = '\0';
		}
		buf1 = bp = alloc_lbuf("process_cmdent.2");
		str = buf2;
		exec(buf1, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL | EV_TOP,
		     &str, cargs, ncargs);
		*bp = '\0';

		if (cmdp->callseq & CS_ARGV) {

			/* Arg2 is ARGV style.  Go get the args */

			parse_arglist(player, cause, arg, '\0',
				      interp | EV_STRIP_LS | EV_STRIP_TS,
				      args, MAX_ARG, cargs, ncargs);
			for (nargs = 0; (nargs < MAX_ARG) && args[nargs]; nargs++) ;

			/* Call the correct command handler */

			if (cmdp->callseq & CS_CMDARG) {
				(*(cmdp->info.handler)) (player, cause, key,
					  buf1, args, nargs, cargs, ncargs);
			} else {
				(*(cmdp->info.handler)) (player, cause, key,
						    buf1, args, nargs);
			}

			/* Free the argument buffers */

			for (i = 0; i <= nargs; i++)
				if (args[i])
					free_lbuf(args[i]);

		} else {

			/* Arg2 is normal style.  Interpret if needed */


			if (interp & EV_EVAL) {
				buf2 = bp = alloc_lbuf("process_cmdent.3");
				str = arg;
				exec(buf2, &bp, 0, player, cause,
				     interp | EV_FCHECK | EV_TOP,
				     &str, cargs, ncargs);
				*bp = '\0';
			} else if (cmdp->callseq & CS_UNPARSE) {
				buf2 = parse_to(&arg, '\0',
					  interp | EV_TOP | EV_NO_COMPRESS);
			} else {
				buf2 = parse_to(&arg, '\0',
				interp | EV_STRIP_LS | EV_STRIP_TS | EV_TOP);
			}

			/* Call the correct command handler */

			if (cmdp->callseq & CS_CMDARG) {
				(*(cmdp->info.handler)) (player, cause, key,
						 buf1, buf2, cargs, ncargs);
			} else {
				(*(cmdp->info.handler)) (player, cause, key,
						    buf1, buf2);
			}

			/* Free the buffer, if needed */

			if (interp & EV_EVAL)
				free_lbuf(buf2);
		}

		/* Free the buffer obtained by evaluating Arg1 */

		free_lbuf(buf1);
		break;
	}

	/* And now we go do the posthook, if we have one. */

	CALL_POST_HOOK(cmdp, cargs, ncargs);

	return;
}

/* ---------------------------------------------------------------------------
 * process_command: Execute a command.
 */

char *process_command(player, cause, interactive, command, args, nargs)
dbref player, cause;
int interactive, nargs;
char *command, *args[];
{
	static char preserve_cmd[LBUF_SIZE];
	char *p, *q, *arg, *lcbuf, *slashp, *cmdsave, *bp, *str, *evcmd;
	int succ, aflags, i, got_stop;
	dbref exit, aowner, parent;
	CMDENT *cmdp;

#ifndef MEMORY_BASED
	cache_reset(0);
#endif /* MEMORY_BASED */

	/* Robustify player */

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *)"< process_command >";

	if (!command)
		abort();

	if (!Good_obj(player)) {
		STARTLOG(LOG_BUGS, "CMD", "PLYR")
			lcbuf = alloc_mbuf("process_command.LOG.badplayer");
		sprintf(lcbuf, "Bad player in process_command: %d",
			player);
		log_text(lcbuf);
		free_mbuf(lcbuf);
		ENDLOG
			mudstate.debug_cmd = cmdsave;
		return command;
	}
	/* Make sure player isn't going or halted */

	if (Going(player) ||
	    (Halted(player) &&
	     !((Typeof(player) == TYPE_PLAYER) && interactive))) {
		notify(Owner(player),
		  tprintf("Attempt to execute command by halted object #%d",
			  player));
		mudstate.debug_cmd = cmdsave;
		return command;
	}

	if (Suspect(player)) {
	    STARTLOG(LOG_SUSPECTCMDS, "CMD", "SUSP")
		log_name_and_loc(player);
	        log_text(" entered: ");
		log_text(command);
	    ENDLOG
	} else {
	    STARTLOG(LOG_ALLCOMMANDS, "CMD", "ALL")
		log_name_and_loc(player);
	        log_text(" entered: ");
		log_text(command);
  	   ENDLOG
       }

	/* Reset recursion limits */

	mudstate.func_nest_lev = 0;
	mudstate.func_invk_ctr = 0;
	mudstate.ntfy_nest_lev = 0;
	mudstate.lock_nest_lev = 0;

	if (Verbose(player))
		notify(Owner(player), tprintf("%s] %s", Name(player),
					      command));

	/*
	 * NOTE THAT THIS WILL BREAK IF "GOD" IS NOT A DBREF.
	 */
	if (mudconf.control_flags & CF_GODMONITOR) {
		raw_notify(GOD, tprintf("%s(#%d)%c %s", Name(player), player,
			(interactive) ? '|' : ':', command));
	}

	/* Eat leading whitespace, and space-compress if configured */

	while (*command && isspace(*command))
		command++;

	strcpy(preserve_cmd, command);
	mudstate.debug_cmd = command;
	mudstate.curr_cmd = preserve_cmd;

	if (mudconf.space_compress) {
		p = q = command;
		while (*p) {
			while (*p && !isspace(*p))
				*q++ = *p++;
			while (*p && isspace(*p))
				p++;
			if (*p)
				*q++ = ' ';
		}
		*q = '\0';
	}
#ifndef MEMORY_BASED
	/* Reset the cache so that unreferenced attributes may be flushed */

	cache_reset(0);
#endif /* MEMORY_BASED */

	/* Now comes the fun stuff.  First check for single-letter leadins.
	 * We check these before checking HOME because
	 * they are among the most frequently executed commands, 
	 * and they can never be the HOME command. 
	 */

	i = command[0] & 0xff;
	if ((prefix_cmds[i] != NULL) && command[0]) {
		process_cmdent(prefix_cmds[i], NULL, player, cause,
			       interactive, command, command, args, nargs);
		mudstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

#ifdef USE_COMSYS
	if (mudconf.have_comsys && !(Slave(player)))
		if (!do_comsystem(player, command))
			return preserve_cmd;
#endif

	/* Check for the HOME command. You cannot do hooks on this because
	 * home is not part of the traditional command table.
	 */

	if (string_compare(command, "home") == 0) {
		if (((Fixed(player)) || (Fixed(Owner(player)))) &&
		    !(WizRoy(player))) {
			notify(player, mudconf.fixed_home_msg);
			mudstate.debug_cmd = cmdsave;
			return preserve_cmd;
		}
		do_move(player, cause, 0, "home");
		mudstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}
	/* Only check for exits if we may use the goto command */

	if (check_access(player, goto_cmdp->perms)) {

		/* Check for an exit name */

		init_match_check_keys(player, command, TYPE_EXIT);
		match_exit_with_parents();
		exit = last_match_result();
		if (exit != NOTHING) {
		    /* Execute the pre-hook for the goto command */
		    CALL_PRE_HOOK(goto_cmdp, args, nargs);
		    move_exit(player, exit, 0, "You can't go that way.", 0);
		    /* Execute the post-hook for the goto command */
		    CALL_POST_HOOK(goto_cmdp, args, nargs);
		    mudstate.debug_cmd = cmdsave;
		    return preserve_cmd;
		}
		
		/* Check for an exit in the master room */

		init_match_check_keys(player, command, TYPE_EXIT);
		match_master_exit();
		exit = last_match_result();
		if (exit != NOTHING) {
		    CALL_PRE_HOOK(goto_cmdp, args, nargs);
		    move_exit(player, exit, 1, NULL, 0);
		    CALL_POST_HOOK(goto_cmdp, args, nargs);
		    mudstate.debug_cmd = cmdsave;
		    return preserve_cmd;
		}
	}
	/* Set up a lowercase command and an arg pointer for the hashed
	 * command check.  Since some types of argument
	 * processing destroy the arguments, make a copy so that
	 * we keep the original command line intact.  Store the
	 * edible copy in lcbuf after the lowercased command. 
	 */
	/* Removed copy of the rest of the command, since it's ok to allow
	 * it to be trashed.  -dcm 
	 */

	lcbuf = alloc_lbuf("process_commands.LCbuf");
	for (p = command, q = lcbuf; *p && !isspace(*p); p++, q++)
		*q = ToLower(*p);	/* Make lowercase command */
	*q++ = '\0';		/* Terminate command */
	while (*p && isspace(*p))
		p++;		/* Skip spaces before arg */
	arg = p;		/* Remember where arg starts */

	/* Strip off any command switches and save them */

	slashp = (char *)index(lcbuf, '/');
	if (slashp)
		*slashp++ = '\0';

	/* Check for a builtin command (or an alias of a builtin command) */

	cmdp = (CMDENT *) hashfind(lcbuf, &mudstate.command_htab);
	if (cmdp != NULL) {
	    if (mudconf.space_compress && (cmdp->callseq & CS_NOSQUISH)) {
		/* We handle this specially -- there is no space compression
		 * involved, so we must go back to the preserved command.
		 */
		strcpy(command, preserve_cmd);
		arg = command;
		while (*arg && !isspace(*arg))
		    arg++;
		if (*arg)     /* we stopped on the space, advance to next */
		    arg++;     
	    }
	    process_cmdent(cmdp, slashp, player, cause, interactive, arg,
			   command, args, nargs);
	    free_lbuf(lcbuf);
	    mudstate.debug_cmd = cmdsave;
	    return preserve_cmd;
	}
	/* Check for enter and leave aliases, user-defined commands on the
	 * player, other objects where the player is, on objects in
	 * the  player's inventory, and on the room that holds 
	 * the player. We evaluate the command line here to allow
	 * chains of $-commands to work. 
	 */

	str = evcmd = alloc_lbuf("process_command.evcmd");
	StringCopy(evcmd, command);
	bp = lcbuf;
	exec(lcbuf, &bp, 0, player, cause,
	     EV_EVAL | EV_FCHECK | EV_STRIP | EV_TOP, &str, args, nargs);
	*bp = '\0';
	free_lbuf(evcmd);
	succ = 0;

	/* Idea for enter/leave aliases from R'nice@TinyTIM */

	if (Has_location(player) && Good_obj(Location(player))) {

	    /* Check for a leave alias, if we have permissions to
	     * use the 'leave' command.
	     */

	    if (check_access(player, leave_cmdp->perms)) {
		p = atr_pget(Location(player), A_LALIAS, &aowner, &aflags);
		if (p && *p) {
		    if (matches_exit_from_list(lcbuf, p)) {
			free_lbuf(lcbuf);
			free_lbuf(p);
			CALL_PRE_HOOK(leave_cmdp, args, nargs);
			do_leave(player, player, 0);
			CALL_POST_HOOK(leave_cmdp, args, nargs);
			return preserve_cmd;
		    }
		}
		free_lbuf(p);
	    }

	    /* Check for enter aliases, if we have permissions to use the
	     * 'enter' command.
	     */

	    if (check_access(player, enter_cmdp->perms)) {
		DOLIST(exit, Contents(Location(player))) {
		    p = atr_pget(exit, A_EALIAS, &aowner, &aflags);
		    if (p && *p) {
			if (matches_exit_from_list(lcbuf, p)) {
			    free_lbuf(lcbuf);
			    free_lbuf(p);
			    CALL_PRE_HOOK(enter_cmdp, args, nargs);
			    do_enter_internal(player, exit, 0);
			    CALL_POST_HOOK(enter_cmdp, args, nargs);
			    return preserve_cmd;
			}
		    }
		    free_lbuf(p);
		}
	    }
	}
	
	/* At each of the following stages, we check to make sure that we
	 * haven't hit a match on a STOP-set object.
	 */
	
	got_stop = 0;
	
	/* Check for $-command matches on me */

	if (mudconf.match_mine) {
		if (((Typeof(player) != TYPE_PLAYER) ||
		     mudconf.match_mine_pl) &&
		    (atr_match(player, player, AMATCH_CMD, lcbuf, preserve_cmd, 1) > 0)) {
			succ++;
			got_stop = Stop_Match(player);
		}
	}
	/* Check for $-command matches on nearby things and on my room */

	if (!got_stop && Has_location(player)) {
		succ += list_check(Contents(Location(player)), player,
				   AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

		if (!got_stop &&
		    atr_match(Location(player), player, AMATCH_CMD, lcbuf,
		              preserve_cmd, 1) > 0) {
			succ++;
			got_stop = Stop_Match(Location(player));
		}
	}
	/* Check for $-command matches in my inventory */

	if (!got_stop && Has_contents(player))
		succ += list_check(Contents(player), player,
				   AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

	/* If we didn't find anything, and we're checking local masters,
	 * do those checks. Do it for the zone of the player's location first,
	 * and then, if nothing is found, on the player's personal zone.
	 * Walking back through the parent tree stops when a match is found.
	 * Also note that these matches are done in the style of the master room:
	 * parents of the contents of the rooms aren't checked for commands.
	 * We try to maintain 2.2/MUX compatibility here, putting both sets
	 * of checks together.
	 */

	/* 2.2 style location */
	
	if (!succ && mudconf.local_masters) {
		if (Has_location(player)) {
			parent = Parent(Location(player));
			while (!succ && !got_stop && Good_obj(parent) && ParentZone(parent)) {
				if (Has_contents(parent)) {
					succ += list_check(Contents(parent), player, AMATCH_CMD,
						lcbuf, preserve_cmd, 0, &got_stop);
				}
				parent = Parent(parent);
			}
		}
	}
	
	/* MUX style location */

	if ((!succ) && mudconf.have_zones &&
	    (Zone(Location(player)) != NOTHING)) {
		if (Typeof(Zone(Location(player))) == TYPE_ROOM) {

			/* zone of player's location is a parent room */
			if (Location(player) != Zone(player)) {

				/* check parent room exits */
				init_match_check_keys(player, command, TYPE_EXIT);
				match_zone_exit();
				exit = last_match_result();
				if (exit != NOTHING) {
				        CALL_PRE_HOOK(goto_cmdp, args, nargs);
					move_exit(player, exit, 1, NULL, 0);
				        CALL_POST_HOOK(goto_cmdp, args, nargs);
					mudstate.debug_cmd = cmdsave;
					return preserve_cmd;
				}
				if (!got_stop) {
					succ += list_check(Contents(Zone(Location(player))), player,
						   AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);
				}
			}	/* end of parent room checks */
		} else
			/* try matching commands on area zone object */

			if (!got_stop && !succ && mudconf.have_zones 
			     && (Zone(Location(player)) != NOTHING)) {
				succ += atr_match(Zone(Location(player)), player, AMATCH_CMD,
					  lcbuf, preserve_cmd, 1);
			}
	}		/* end of matching on zone of player's location */
	
	/* 2.2 style player */
	
	if (!succ && mudconf.local_masters) {
		parent = Parent(player);
		if ((parent != Location(player)) &&
		    (!Good_obj(Location(player)) ||
		     (parent != Parent(Location(player))))) {
			while (!succ && !got_stop &&
			       Good_obj(parent) && ParentZone(parent)) {
				if (Has_contents(parent)) {
					succ += list_check(Contents(parent), player,
						AMATCH_CMD, lcbuf, preserve_cmd, 0,
						&got_stop);
				}
				parent = Parent(parent);
			}
		}
	}

	/* MUX style player */
	
	/* if nothing matched with parent room/zone object, try matching
	 * zone commands on the player's personal zone  
	 */
	if (!got_stop && !succ && mudconf.have_zones && (Zone(player) != NOTHING) &&
	    (Zone(Location(player)) != Zone(player))) {
		succ += atr_match(Zone(player), player, AMATCH_CMD, lcbuf, 
			preserve_cmd, 1);
	}
	/* If we didn't find anything, try in the master room */

	if (!got_stop && !succ) {
		if (Good_obj(mudconf.master_room) &&
		    Has_contents(mudconf.master_room)) {
			succ += list_check(Contents(mudconf.master_room),
					   player, AMATCH_CMD, lcbuf,
					   preserve_cmd, 0, &got_stop);
			if (!got_stop && atr_match(mudconf.master_room,
			     player, AMATCH_CMD, lcbuf, preserve_cmd, 0) > 0) {
				succ++;
			}
		}
	}
	free_lbuf(lcbuf);

	/* If we still didn't find anything, tell how to get help. */

	if (!succ) {
		notify(player, "Huh?  (Type \"help\" for help.)");
		STARTLOG(LOG_BADCOMMANDS, "CMD", "BAD")
			log_name_and_loc(player);
			log_text((char *) " entered: ");
			log_text(command);
		ENDLOG
	}
	mudstate.debug_cmd = cmdsave;
	return preserve_cmd;
}

/* ---------------------------------------------------------------------------
 * list_cmdtable: List internal commands.
 */

static void list_cmdtable(player)
dbref player;
{
	CMDENT *cmdp;
	char *buf, *bp, *cp;

	buf = alloc_lbuf("list_cmdtable");
	bp = buf;
	for (cp = (char *)"Commands:"; *cp; cp++)
		*bp++ = *cp;
	for (cmdp = command_table; cmdp->cmdname; cmdp++) {
		if (check_access(player, cmdp->perms)) {
			if (!(cmdp->perms & CF_DARK)) {
				*bp++ = ' ';
				for (cp = cmdp->cmdname; *cp; cp++)
					*bp++ = *cp;
			}
		}
	}
	*bp = '\0';

	/* Players get the list of logged-out cmds too */

	if (Typeof(player) == TYPE_PLAYER)
		display_nametab(player, logout_cmdtable, buf, 1);
	else
		notify(player, buf);
	free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * list_attrtable: List available attributes.
 */

static void list_attrtable(player)
dbref player;
{
	ATTR *ap;
	char *buf, *bp, *cp;

	buf = alloc_lbuf("list_attrtable");
	bp = buf;
	for (cp = (char *)"Attributes:"; *cp; cp++)
		*bp++ = *cp;
	for (ap = attr; ap->name; ap++) {
		if (See_attr(player, player, ap, player, 0)) {
			*bp++ = ' ';
			for (cp = (char *)(ap->name); *cp; cp++)
				*bp++ = *cp;
		}
	}
	*bp = '\0';
	raw_notify(player, buf);
	free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * list_cmdaccess: List access commands.
 */

NAMETAB access_nametab[] =
{
	{(char *)"builder", 6, CA_WIZARD, CA_BUILDER},
	{(char *)"dark", 4, CA_GOD, CF_DARK},
	{(char *)"disabled", 4, CA_GOD, CA_DISABLED},
	{(char *)"global_build", 8, CA_PUBLIC, CA_GBL_BUILD},
	{(char *)"global_interp", 8, CA_PUBLIC, CA_GBL_INTERP},
	{(char *)"god", 2, CA_GOD, CA_GOD},
	{(char *)"head", 2, CA_WIZARD, CA_HEAD},
	{(char *)"immortal", 3, CA_WIZARD, CA_IMMORTAL},
	{(char *)"marker0", 7, CA_WIZARD, CA_MARKER0},
	{(char *)"marker1", 7, CA_WIZARD, CA_MARKER1},
	{(char *)"marker2", 7, CA_WIZARD, CA_MARKER2},
	{(char *)"marker3", 7, CA_WIZARD, CA_MARKER3},
	{(char *)"marker4", 7, CA_WIZARD, CA_MARKER4},
	{(char *)"marker5", 7, CA_WIZARD, CA_MARKER5},
	{(char *)"marker6", 7, CA_WIZARD, CA_MARKER6},
	{(char *)"marker7", 7, CA_WIZARD, CA_MARKER7},
	{(char *)"marker8", 7, CA_WIZARD, CA_MARKER8},
	{(char *)"marker9", 7, CA_WIZARD, CA_MARKER9},
	{(char *)"need_location", 6, CA_PUBLIC, CA_LOCATION},
	{(char *)"need_contents", 6, CA_PUBLIC, CA_CONTENTS},
	{(char *)"need_player", 6, CA_PUBLIC, CA_PLAYER},
	{(char *)"no_haven", 4, CA_PUBLIC, CA_NO_HAVEN},
	{(char *)"no_robot", 4, CA_WIZARD, CA_NO_ROBOT},
	{(char *)"no_slave", 5, CA_PUBLIC, CA_NO_SLAVE},
	{(char *)"no_suspect", 5, CA_WIZARD, CA_NO_SUSPECT},
	{(char *)"no_guest", 5, CA_WIZARD, CA_NO_GUEST},
	{(char *)"sql", 2, CA_GOD, CA_SQL_OK},
	{(char *)"staff", 3, CA_WIZARD, CA_STAFF},
	{(char *)"static", 3, CA_GOD, CA_STATIC},
	{(char *)"wizard", 3, CA_WIZARD, CA_WIZARD},
	{NULL, 0, 0, 0}};

static void list_cmdaccess(player)
dbref player;
{
	char *buff, *p, *q;
	CMDENT *cmdp;
	ATTR *ap;

	buff = alloc_sbuf("list_cmdaccess");
	for (cmdp = command_table; cmdp->cmdname; cmdp++) {
		if (check_access(player, cmdp->perms)) {
			if (!(cmdp->perms & CF_DARK)) {
				sprintf(buff, "%s:", cmdp->cmdname);
				listset_nametab(player, access_nametab,
						cmdp->perms, buff, 1);
			}
		}
	}
	for (ap = attr; ap->name; ap++) {
		p = buff;
		*p++ = '@';
		for (q = (char *)ap->name; *q; p++, q++)
			*p = ToLower(*q);
		if (ap->flags & AF_NOCMD)
			continue;
		*p = '\0';
		cmdp = (CMDENT *) hashfind(buff, &mudstate.command_htab);
		if (cmdp == NULL)
			continue;
		if (!check_access(player, cmdp->perms))
			continue;
		if (!(cmdp->perms & CF_DARK)) {
			sprintf(buff, "%s:", cmdp->cmdname);
			listset_nametab(player, access_nametab,
					cmdp->perms, buff, 1);
		}
	}
	free_sbuf(buff);
}

/* ---------------------------------------------------------------------------
 * list_cmdswitches: List switches for commands.
 */

static void list_cmdswitches(player)
dbref player;
{
	char *buff;
	CMDENT *cmdp;

	buff = alloc_sbuf("list_cmdswitches");
	for (cmdp = command_table; cmdp->cmdname; cmdp++) {
		if (cmdp->switches) {
			if (check_access(player, cmdp->perms)) {
				if (!(cmdp->perms & CF_DARK)) {
					sprintf(buff, "%s:", cmdp->cmdname);
					display_nametab(player, cmdp->switches,
							buff, 0);
				}
			}
		}
	}
	free_sbuf(buff);
}
/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * list_attraccess: List access to attributes.
 */

NAMETAB attraccess_nametab[] = {
{(char *)"dark",		2,	CA_WIZARD,	AF_DARK},
{(char *)"deleted",		2,	CA_WIZARD,	AF_DELETED},
{(char *)"god",			1,	CA_PUBLIC,	AF_GOD},
{(char *)"hidden",		1,	CA_WIZARD,	AF_MDARK},
{(char *)"html",		2,	CA_PUBLIC,	AF_HTML},
{(char *)"ignore",		2,	CA_WIZARD,	AF_NOCMD},
{(char *)"internal",		2,	CA_WIZARD,	AF_INTERNAL},
{(char *)"is_lock",		4,	CA_PUBLIC,	AF_IS_LOCK},
{(char *)"locked",		1,	CA_PUBLIC,	AF_LOCK},
{(char *)"no_command",		4,	CA_PUBLIC,	AF_NOPROG},
{(char *)"no_inherit",		4,	CA_PUBLIC,	AF_PRIVATE},
{(char *)"no_parse",		4,	CA_PUBLIC,	AF_NOPARSE},
{(char *)"private",		1,	CA_PUBLIC,	AF_ODARK},
{(char *)"regexp", 		1,	CA_PUBLIC,	AF_REGEXP},
{(char *)"visual",		1,	CA_PUBLIC,	AF_VISUAL},
{(char *)"wizard",		1,	CA_PUBLIC,	AF_WIZARD},
{ NULL,				0,	0,		0}};

NAMETAB indiv_attraccess_nametab[] = {
{(char *)"hidden",		1,	CA_WIZARD,	AF_MDARK},
{(char *)"wizard",		1,	CA_WIZARD,	AF_WIZARD},
{(char *)"no_command",		4,	CA_PUBLIC,	AF_NOPROG},
{(char *)"no_inherit",		4,	CA_PUBLIC,	AF_PRIVATE},
{(char *)"no_parse",		4,	CA_PUBLIC,	AF_NOPARSE},
{(char *)"visual",		1,	CA_PUBLIC,	AF_VISUAL},
{(char *)"regexp", 		1,	CA_PUBLIC,	AF_REGEXP},
{(char *)"html",		2,	CA_PUBLIC,	AF_HTML},
{ NULL,				0,	0,		0}};

/* *INDENT-ON* */

static void list_attraccess(player)
dbref player;
{
	char *buff;
	ATTR *ap;

	buff = alloc_sbuf("list_attraccess");
	for (ap = attr; ap->name; ap++) {
		if (Read_attr(player, player, ap, player, 0)) {
			sprintf(buff, "%s:", ap->name);
			listset_nametab(player, attraccess_nametab,
					ap->flags, buff, 1);
		}
	}
	free_sbuf(buff);
}

/* ---------------------------------------------------------------------------
 * cf_access: Change command or switch permissions.
 */

extern void FDECL(cf_log_notfound, (dbref, char *, const char *, char *));

CF_HAND(cf_access)
{
	CMDENT *cmdp;
	char *ap;
	int set_switch;

	for (ap = str; *ap && !isspace(*ap) && (*ap != '/'); ap++) ;
	if (*ap == '/') {
		set_switch = 1;
		*ap++ = '\0';
	} else {
		set_switch = 0;
		if (*ap)
			*ap++ = '\0';
		while (*ap && isspace(*ap))
			ap++;
	}

	cmdp = (CMDENT *) hashfind(str, &mudstate.command_htab);
	if (cmdp != NULL) {
		if (set_switch)
			return cf_ntab_access((int *)cmdp->switches, ap,
					      extra, player, cmd);
		else
			return cf_modify_bits(&(cmdp->perms), ap,
					      extra, player, cmd);
	} else {
		cf_log_notfound(player, cmd, "Command", str);
		return -1;
	}
}

/* ---------------------------------------------------------------------------
 * cf_acmd_access: Chante command permissions for all attr-setting cmds.
 */

CF_HAND(cf_acmd_access)
{
	CMDENT *cmdp;
	ATTR *ap;
	char *buff, *p, *q;
	int failure, save;

	buff = alloc_sbuf("cf_acmd_access");
	for (ap = attr; ap->name; ap++) {
		p = buff;
		*p++ = '@';
		for (q = (char *)ap->name; *q; p++, q++)
			*p = ToLower(*q);
		*p = '\0';
		cmdp = (CMDENT *) hashfind(buff, &mudstate.command_htab);
		if (cmdp != NULL) {
			save = cmdp->perms;
			failure = cf_modify_bits(&(cmdp->perms), str,
						 extra, player, cmd);
			if (failure != 0) {
				cmdp->perms = save;
				free_sbuf(buff);
				return -1;
			}
		}
	}
	free_sbuf(buff);
	return 0;
}

/* ---------------------------------------------------------------------------
 * cf_attr_access: Change access on an attribute.
 */

CF_HAND(cf_attr_access)
{
	ATTR *ap;
	char *sp;

	for (sp = str; *sp && !isspace(*sp); sp++) ;
	if (*sp)
		*sp++ = '\0';
	while (*sp && isspace(*sp))
		sp++;

	ap = atr_str(str);
	if (ap != NULL)
		return cf_modify_bits(&(ap->flags), sp, extra, player, cmd);
	else {
		cf_log_notfound(player, cmd, "Attribute", str);
		return -1;
	}
}

/* ---------------------------------------------------------------------------
 * cf_cmd_alias: Add a command alias.
 */

CF_HAND(cf_cmd_alias)
{
	char *alias, *orig, *ap;
	CMDENT *cmdp, *cmd2;
	NAMETAB *nt;
	int *hp;

	alias = strtok(str, " \t=,");
	orig = strtok(NULL, " \t=,");

	if (!orig)		/* we only got one argument to @alias. Bad. */
		return -1;

	for (ap = orig; *ap && (*ap != '/'); ap++) ;
	if (*ap == '/') {

		/* Switch form of command aliasing: create an alias for a
		 * command + a switch 
		 */

		*ap++ = '\0';

		/* Look up the command */

		cmdp = (CMDENT *) hashfind(orig, (HASHTAB *) vp);
		if (cmdp == NULL) {
			cf_log_notfound(player, cmd, "Command", orig);
			return -1;
		}
		/* Look up the switch */

		nt = find_nametab_ent(player, (NAMETAB *) cmdp->switches, ap);
		if (!nt) {
			cf_log_notfound(player, cmd, "Switch", ap);
			return -1;
		}
		/*
		 * Got it, create the new command table entry 
		 */

		cmd2 = (CMDENT *) XMALLOC(sizeof(CMDENT), "cf_cmd_alias");
		cmd2->cmdname = strsave(alias);
		cmd2->switches = cmdp->switches;
		cmd2->perms = cmdp->perms | nt->perm;
		cmd2->extra = (cmdp->extra | nt->flag) & ~SW_MULTIPLE;
		if (!(nt->flag & SW_MULTIPLE))
			cmd2->extra |= SW_GOT_UNIQUE;
		cmd2->callseq = cmdp->callseq;

		/*
		 * KNOWN PROBLEM:
		 * We are not inheriting the hook that the 'original' command
		 * had -- we will have to add it manually (whereas an alias
		 * of a non-switched command is just another hashtable entry
		 * for the same command pointer and therefore gets the hook).
		 * This is preferable to having to search the hashtable for
		 * hooks when a hook is deleted, though.
		 */
		cmd2->pre_hook = NULL;
		cmd2->post_hook = NULL;

		cmd2->info.handler = cmdp->info.handler;
		if (hashadd(cmd2->cmdname, (int *)cmd2, (HASHTAB *) vp)) {
			XFREE(cmd2->cmdname, "cf_cmd_alias.2");
			XFREE(cmd2, "cf_cmd_alias.3");
		}
	} else {

		/* A normal (non-switch) alias */

		hp = hashfind(orig, (HASHTAB *) vp);
		if (hp == NULL) {
			cf_log_notfound(player, cmd, "Entry", orig);
			return -1;
		}
		hashadd(alias, hp, (HASHTAB *) vp);
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * list_df_flags: List default flags at create time.
 */

static void list_df_flags(player)
dbref player;
{
	char *playerb, *roomb, *thingb, *exitb, *robotb, *buff;

	playerb = decode_flags(player,
			       (mudconf.player_flags.word1 | TYPE_PLAYER),
			       mudconf.player_flags.word2,
			       mudconf.player_flags.word3);
	roomb = decode_flags(player,
			     (mudconf.room_flags.word1 | TYPE_ROOM),
			     mudconf.room_flags.word2,
			     mudconf.room_flags.word3);
	exitb = decode_flags(player,
			     (mudconf.exit_flags.word1 | TYPE_EXIT),
			     mudconf.exit_flags.word2,
			     mudconf.exit_flags.word3);
	thingb = decode_flags(player,
			      (mudconf.thing_flags.word1 | TYPE_THING),
			      mudconf.thing_flags.word2,
			      mudconf.thing_flags.word3);
	robotb = decode_flags(player,
			      (mudconf.robot_flags.word1 | TYPE_PLAYER),
			      mudconf.robot_flags.word2,
			      mudconf.robot_flags.word3);
	buff = alloc_lbuf("list_df_flags");
	sprintf(buff,
		"Default flags: Players...%s Rooms...%s Exits...%s Things...%s Robots...%s",
		playerb, roomb, exitb, thingb, robotb);
	raw_notify(player, buff);
	free_lbuf(buff);
	free_sbuf(playerb);
	free_sbuf(roomb);
	free_sbuf(exitb);
	free_sbuf(thingb);
	free_sbuf(robotb);
}

/* ---------------------------------------------------------------------------
 * list_costs: List the costs of things.
 */

#define coin_name(s)	(((s)==1) ? mudconf.one_coin : mudconf.many_coins)

static void list_costs(player)
dbref player;
{
	char *buff;

	buff = alloc_mbuf("list_costs");
	*buff = '\0';
	if (mudconf.quotas)
		sprintf(buff, " and %d quota", mudconf.room_quota);
	notify(player,
	       tprintf("Digging a room costs %d %s%s.",
		       mudconf.digcost, coin_name(mudconf.digcost), buff));
	if (mudconf.quotas)
		sprintf(buff, " and %d quota", mudconf.exit_quota);
	notify(player,
	       tprintf("Opening a new exit costs %d %s%s.",
		       mudconf.opencost, coin_name(mudconf.opencost), buff));
	notify(player,
	       tprintf("Linking an exit, home, or dropto costs %d %s.",
		       mudconf.linkcost, coin_name(mudconf.linkcost)));
	if (mudconf.quotas)
		sprintf(buff, " and %d quota", mudconf.thing_quota);
	if (mudconf.createmin == mudconf.createmax)
		raw_notify(player,
			   tprintf("Creating a new thing costs %d %d %s%s.",
				   mudconf.createmin,
				   coin_name(mudconf.createmin), buff));
	else
		raw_notify(player,
		tprintf("Creating a new thing costs between %d and %d %s%s.",
			mudconf.createmin, mudconf.createmax,
			mudconf.many_coins, buff));
	if (mudconf.quotas)
		sprintf(buff, " and %d quota", mudconf.player_quota);
	notify(player,
	       tprintf("Creating a robot costs %d %s%s.",
		       mudconf.robotcost, coin_name(mudconf.robotcost),
		       buff));
	if (mudconf.killmin == mudconf.killmax) {
		raw_notify(player,
			   tprintf("Killing costs %d %s, with a %d%% chance of success.",
				mudconf.killmin, coin_name(mudconf.digcost),
				   (mudconf.killmin * 100) /
				   mudconf.killguarantee));
	} else {
		raw_notify(player,
			   tprintf("Killing costs between %d and %d %s.",
				   mudconf.killmin, mudconf.killmax,
				   mudconf.many_coins));
		raw_notify(player,
		       tprintf("You must spend %d %s to guarantee success.",
			       mudconf.killguarantee,
			       coin_name(mudconf.killguarantee)));
	}
	raw_notify(player,
		   tprintf("Computationally expensive commands and functions (ie: @entrances, @find, @search, @stats (with an argument or switch), search(), and stats()) cost %d %s.",
			mudconf.searchcost, coin_name(mudconf.searchcost)));
	if (mudconf.machinecost > 0)
		raw_notify(player,
		   tprintf("Each command run from the queue costs 1/%d %s.",
			   mudconf.machinecost, mudconf.one_coin));
	if (mudconf.waitcost > 0) {
		raw_notify(player,
			   tprintf("A %d %s deposit is charged for putting a command on the queue.",
				   mudconf.waitcost, mudconf.one_coin));
		raw_notify(player, "The deposit is refunded when the command is run or canceled.");
	}
	if (mudconf.sacfactor == 0)
		sprintf(buff, "%d", mudconf.sacadjust);
	else if (mudconf.sacfactor == 1) {
		if (mudconf.sacadjust < 0)
			sprintf(buff, "<create cost> - %d", -mudconf.sacadjust);
		else if (mudconf.sacadjust > 0)
			sprintf(buff, "<create cost> + %d", mudconf.sacadjust);
		else
			sprintf(buff, "<create cost>");
	} else {
		if (mudconf.sacadjust < 0)
			sprintf(buff, "(<create cost> / %d) - %d",
				mudconf.sacfactor, -mudconf.sacadjust);
		else if (mudconf.sacadjust > 0)
			sprintf(buff, "(<create cost> / %d) + %d",
				mudconf.sacfactor, mudconf.sacadjust);
		else
			sprintf(buff, "<create cost> / %d", mudconf.sacfactor);
	}
	raw_notify(player, tprintf("The value of an object is %s.", buff));
	if (mudconf.clone_copy_cost)
		raw_notify(player, "The default value of cloned objects is the value of the original object.");
	else
		raw_notify(player,
		    tprintf("The default value of cloned objects is %d %s.",
			    mudconf.createmin,
			    coin_name(mudconf.createmin)));

	free_mbuf(buff);
}

/* ---------------------------------------------------------------------------
 * list_options: List more game options from mudconf.
 */

static const char *switchd[] =
{"/first", "/all"};
static const char *examd[] =
{"/brief", "/full"};
static const char *ed[] =
{"Disabled", "Enabled"};

static void list_options(player)
dbref player;
{
	char *buff;
	time_t now;

	now = time(NULL);

	/* --- Config options related to Building --- */

	if (mudconf.quotas)
	    raw_notify(player, "Building quotas are enforced.");
	else
	    raw_notify(player, "Building quotes are not enforced.");

	if (mudconf.typed_quotas)
	    raw_notify(player, "Quotas are managed by object type.");

	raw_notify(player, tprintf("There is a limit of %d objects in the database.",
				   mudconf.building_limit));

	if (mudconf.name_spaces)
	    raw_notify(player, "Player names may contain spaces.");
	else
	    raw_notify(player, "Player names may not contain spaces.");

	raw_notify(player,
		   tprintf("Default Parents:  Room...#%d  Exit...#%d  Thing...#%d  Player...#%d",
			   mudconf.room_parent, mudconf.exit_parent,
			   mudconf.thing_parent, mudconf.player_parent));

	/* --- Config options related to Programming. -- */

	raw_notify(player,
		   tprintf("Players may have at most %d commands in the queue at one time.",
			   mudconf.queuemax));

	if (mudconf.req_cmds_flag)
	    raw_notify(player, "Objects are only searched for $-commands if set COMMANDS.");

	if (mudconf.match_mine) {
	    if (mudconf.match_mine_pl)
		raw_notify(player, "All objects search themselves for $-commands.");
	    else
		raw_notify(player, "Objects other than players search themselves for $-commands.");
	} else {
	    raw_notify(player, "Objects do not search themselves for $-commands.");
	}
	
#ifdef NO_LAG_CHECK
        raw_notify(player, "CPU usage warnings are disabled.");
#else
        raw_notify(player, "CPU usage warnings are enabled.");
#endif /* NO_LAG_CHECK */

#ifdef FLOATING_POINTS
	raw_notify(player, "MUSH arithmetic operations use floating-point numbers.");
#else
        raw_notify(player, "MUSH arithmetic operations use integers.");
#endif                          /* FLOATING_POINTS */

	if (mudconf.trace_topdown) {
		raw_notify(player, "Trace output is presented top-down (whole expression first, then sub-exprs).");
		raw_notify(player, tprintf("Only %d lines of trace output are displayed.",
					   mudconf.trace_limit));
	} else {
		raw_notify(player, "Trace output is presented bottom-up (subexpressions first).");
	}

#ifdef PUEBLO_SUPPORT
	raw_notify(player, "Pueblo client extensions are supported.");
#endif

	if (mudconf.fascist_tport)
		raw_notify(player, "You may only @teleport out of locations that are JUMP_OK or that you control.");

	if (mudconf.safer_passwords)
	    notify(player, "Passwords cannot be easily guessable.");

	/* --- Config options related to Speaking --- */

	if (mudconf.pemit_players)
	    raw_notify(player, "The '@pemit' command may be used to emit to faraway players.");
	else
	    raw_notify(player, "The '@pemit' command may not be used to emit to faraway players.");

	if (mudconf.pemit_any)
	    raw_notify(player, "The '@pemit' command may be used to emit to any remote object.");
	else
	    raw_notify(player, "The '@pemit' command may not be used to emit to any remote object.");

	if (mudconf.player_listen)
	    raw_notify(player, "The @Listen/@Ahear attribute set works on player objects.");
	else
	    raw_notify(player, "The @Listen/@Ahear attribute set does not work on player objects.");

	if (!mudconf.quiet_whisper)
		raw_notify(player, "The 'whisper' command lets others in the room with you know you whispered.");

	if (!mudconf.robot_speak)
		raw_notify(player, "Robots are not allowed to speak in public areas.");

	/* --- Default Command Behaviors --- */

	raw_notify(player,
	      tprintf("The default switch for the '@switch' command is %s.",
		      switchd[mudconf.switch_df_all]));

	raw_notify(player,
	      tprintf("The default switch for the 'examine' command is %s.",
		      examd[mudconf.exam_public]));

	if (mudconf.lattr_oldstyle) {
	    raw_notify(player,
		       "The lattr() function returns an empty string on a bad match.");
	} else {
	    raw_notify(player,
		       "The lattr() function returns #-1 NO MATCH on a bad match.");
	}

	/* --- Config options related to Looking and other information --- */

	if (mudconf.ex_flags)
		raw_notify(player, "The 'examine' command lists the flag names for the object's flags.");
	if (mudconf.pub_flags)
		raw_notify(player, "The 'flags()' function will return the flags of any object.");

	if (mudconf.read_rem_desc)
		raw_notify(player, "The 'get()' function will return the description of faraway objects,");
	if (mudconf.read_rem_name)
		raw_notify(player, "The 'name()' function will return the name of faraway objects.");

	if (!mudconf.quiet_look)
		raw_notify(player, "The 'look' command shows visible attributes in addition to the description.");
	if (mudconf.see_own_dark)
		raw_notify(player, "The 'look' command lists DARK objects owned by you.");
	if (!mudconf.dark_sleepers)
		raw_notify(player, "The 'look' command shows disconnected players.");

	if (mudconf.terse_look)
		raw_notify(player, "The 'look' command obeys the TERSE flag.");
	if (!mudconf.terse_contents)
		raw_notify(player, "The TERSE flag suppresses listing the contents of a location.");
	if (!mudconf.terse_exits)
		raw_notify(player, "The TERSE flag suppresses listing obvious exits in a location.");
	if (!mudconf.terse_movemsg)
		raw_notify(player, "The TERSE flag suppresses enter/leave/succ/drop messages generated by moving.");

	if (mudconf.fmt_contents)
	    raw_notify(player, "The format of Contents can be specified with @conformat.");
	if (mudconf.fmt_exits)
	    raw_notify(player, "The format of Exits can be specified with @exitformat.");

	if (mudconf.ansi_colors)
	    raw_notify(player, "ANSI sequences can be used in text.");

	if (mudconf.sweep_dark)
		raw_notify(player, "Players may @sweep dark locations.");

#ifdef USE_MAIL
	if (mudconf.have_mailer)
		raw_notify(player, "The built-in @mail system is enabled.");
	else
		raw_notify(player, "The built-in @mail system is disabled.");
#endif
#ifdef USE_COMSYS
	if (mudconf.have_comsys)
		raw_notify(player, "The built-in comsystem is enabled.");
	else
		raw_notify(player, "The built-in comsystem is disabled.");
#endif

#ifdef TCL_INTERP_SUPPORT
	notify(player, "TCL interpreter support is enabled.");
#endif /* TCL_INTERP_SUPPORT */

	if (!Wizard(player))
		return;
	buff = alloc_mbuf("list_options");

	raw_notify(player,
		   tprintf("%d commands are run from the queue when there is no net activity.",
			   mudconf.queue_chunk));
	raw_notify(player,
		   tprintf("%d commands are run from the queue when there is net activity.",
			   mudconf.active_q_chunk));

	if (mudconf.idle_wiz_dark)
		raw_notify(player, "Wizards idle for longer than the default timeout are automatically set DARK.");

	if (mudconf.safe_unowned)
		raw_notify(player, "Objects not owned by you are automatically considered SAFE.");

	if (mudconf.paranoid_alloc)
		raw_notify(player, "The buffer pools are checked for consistency on each allocate or free.");

	raw_notify(player,
	      tprintf("The %s cache is %d entries wide by %d entries deep.",
		      CACHING, mudconf.cache_width, mudconf.cache_depth));

	if (mudconf.cache_names)
		raw_notify(player, "A seperate name cache is used.");

	if (mudconf.cache_trim)
		raw_notify(player, "The cache depth is periodically trimmed back to its initial value.");

	if (mudconf.fork_dump) {
		raw_notify(player, "Database dumps are performed by a fork()ed process.");
		if (mudconf.fork_vfork)
			raw_notify(player, "The 'vfork()' call is used to perform the fork.");
	}

	if (mudconf.use_global_aconn)
		raw_notify(player, "Global aconnects and disconnects are executed by Master Room objects.");
	if (mudconf.global_aconn_uselocks)
	        raw_notify(player, "Global aconnects and disconnects obey Uselocks.");

	if (mudconf.local_masters) {
		raw_notify(player, "Objects set ZONE are treated as local master rooms.");
	}
	if (mudconf.have_zones) {
		raw_notify(player, "ControlLock zones are enabled.");
	}

	if (mudconf.addcmd_obey_uselocks) {
	    raw_notify(player, "Matches on added commands obey uselocks.");
	} else {
	    raw_notify(player, "Matches on added commands do not obey uselocks.");
	}

	if (mudconf.addcmd_match_blindly) {
	    raw_notify(player, "Failure to correctly match syntax on an @addcommand does not produce an error message.");
	} else {
	    raw_notify(player, "Failure to correctly match syntax on an @addcommand produces an error message.");
	}

	if (mudconf.addcmd_obey_stop) {
	    raw_notify(player, "Matching an @addcommand on an object set STOP ends matching attempts.");
	} else {
	    raw_notify(player, "Matching an @addcommand on an object set STOP does not end matching attempts.");
	}

	/* Only display SQL db status if we seem to be configured for one.
	 * Do NOT display where the database is. Wizards don't need to know,
	 * and it can never change dynamically, only in the conf file.
	 */
	if (mudconf.sql_host && *mudconf.sql_host) {
	    if (mudstate.sql_socket != -1) {
		raw_notify(player, "There is an open connection to an external SQL database.");
	    } else {
		raw_notify(player, "There is no open connection to an external SQL database.");
	    }
	    if (mudconf.sql_reconnect != 0) {
		raw_notify(player, "SQL queries re-initiate dropped connections.");
	    } else {
		raw_notify(player, "SQL queries do not re-initiate dropped connections.");
	    }
	}

	if (mudconf.max_players >= 0)
		raw_notify(player,
		tprintf("There may be at most %d players logged in at once.",
			mudconf.max_players));

	raw_notify(player,
		   tprintf("New players are given %d %s to start with.",
			   mudconf.paystart, mudconf.many_coins));
	raw_notify(player,
		   tprintf("Players are given %d %s each day they connect.",
			   mudconf.paycheck, mudconf.many_coins));
	raw_notify(player,
	  tprintf("Earning money is difficult if you have more than %d %s.",
		  mudconf.paylimit, mudconf.many_coins));
	if (mudconf.payfind > 0)
		raw_notify(player,
			   tprintf("Players have a 1 in %d chance of finding a %s each time they move.",
				   mudconf.payfind, mudconf.one_coin));

	raw_notify(player,
		   tprintf("The head of the object freelist is #%d.",
			   mudstate.freelist));

	sprintf(buff, "Intervals: Dump...%d  Clean...%d  Idlecheck...%d",
		mudconf.dump_interval, mudconf.check_interval,
		mudconf.idle_interval);
	raw_notify(player, buff);

	sprintf(buff, "Timers: Dump...%d  Clean...%d  Idlecheck...%d",
		mudstate.dump_counter - now, mudstate.check_counter - now,
		mudstate.idle_counter - now);
	raw_notify(player, buff);

	sprintf(buff,
		"Timeouts: Idle...%d  Connect...%d  Tries...%d  Lag...%d",
		mudconf.idle_timeout, mudconf.conn_timeout,
		mudconf.retry_limit, mudconf.max_cmdsecs);
	raw_notify(player, buff);

	sprintf(buff, "Scheduling: Timeslice...%d  Max_Quota...%d  Increment...%d",
		mudconf.timeslice, mudconf.cmd_quota_max,
		mudconf.cmd_quota_incr);
	raw_notify(player, buff);

	sprintf(buff, "Compression: Spaces...%s  SaveFiles...%s",
		ed[mudconf.space_compress], ed[mudconf.compress_db]);
	raw_notify(player, buff);

	sprintf(buff, "MasterRoom...#%d  StartRoom...#%d  StartHome...#%d  DefaultHome...#%d",
		mudconf.master_room, mudconf.start_room, mudconf.start_home,
		mudconf.default_home);
	raw_notify(player, buff);

	sprintf(buff, "New Characters: %s...%d  Quota...%d  Rooms...%d  Exits...%d  Things...%d  Players...%d",
		mudconf.many_coins, mudconf.paystart, mudconf.start_quota,
		mudconf.start_room_quota, mudconf.start_exit_quota,
		mudconf.start_thing_quota, mudconf.start_player_quota);
	raw_notify(player, buff);

	sprintf(buff, "Misc: GuestChar...#%d", mudconf.guest_char);
	raw_notify(player, buff);

	sprintf(buff, "Limits: Output...%d  Recursion...%d  Invocation...%d  Parents...%d  Stacks...%d  Variables...%d  Structures...%d  Instances...%d",
		mudconf.output_limit, mudconf.func_nest_lim,
		mudconf.func_invk_lim, mudconf.parent_nest_lim,
		mudconf.stack_lim, mudconf.numvars_lim,
		mudconf.struct_lim, mudconf.instance_lim);
	raw_notify(player, buff);

	free_mbuf(buff);
}

/* ---------------------------------------------------------------------------
 * list_vattrs: List user-defined attributes
 */

static void list_vattrs(player)
dbref player;
{
	VATTR *va;
	int na;
	char *buff;

	buff = alloc_lbuf("list_vattrs");
	raw_notify(player, "--- User-Defined Attributes ---");
	for (va = vattr_first(), na = 0; va; va = vattr_next(va), na++) {
		if (!(va->flags & AF_DELETED)) {
			sprintf(buff, "%s(%d):", va->name, va->number);
			listset_nametab(player, attraccess_nametab, va->flags,
					buff, 1);
		}
	}

	raw_notify(player, tprintf("%d attributes, next=%d",
				   na, mudstate.attr_next));
	free_lbuf(buff);
}

/* ---------------------------------------------------------------------------
 * list_hashstats: List information from hash tables
 */

static void list_hashstat(player, tab_name, htab)
dbref player;
HASHTAB *htab;
const char *tab_name;
{
	char *buff;

	buff = hashinfo(tab_name, htab);
	raw_notify(player, buff);
	free_mbuf(buff);
}

static void list_nhashstat(player, tab_name, htab)
dbref player;
NHSHTAB *htab;
const char *tab_name;
{
	char *buff;

	buff = nhashinfo(tab_name, htab);
	raw_notify(player, buff);
	free_mbuf(buff);
}

static void list_hashstats(player)
dbref player;
{
	raw_notify(player, "Hash Stats       Size Entries Deleted   Empty Lookups    Hits  Checks Longest");
	list_hashstat(player, "Commands", &mudstate.command_htab);
	list_hashstat(player, "Logged-out Cmds", &mudstate.logout_cmd_htab);
	list_hashstat(player, "Functions", &mudstate.func_htab);
	list_hashstat(player, "User Functions", &mudstate.ufunc_htab);
	list_hashstat(player, "Flags", &mudstate.flags_htab);
	list_hashstat(player, "Powers", &mudstate.powers_htab);
	list_hashstat(player, "Attr names", &mudstate.attr_name_htab);
	list_hashstat(player, "Vattr names", &mudstate.vattr_name_htab);
	list_hashstat(player, "Player Names", &mudstate.player_htab);
	list_nhashstat(player, "Net Descriptors", &mudstate.desc_htab);
	list_nhashstat(player, "Forwardlists", &mudstate.fwdlist_htab);
	list_nhashstat(player, "Overlaid $-cmds", &mudstate.parent_htab);
	list_nhashstat(player, "Object Stacks", &mudstate.objstack_htab);
	list_hashstat(player, "Variables", &mudstate.vars_htab);
	list_hashstat(player, "Structure Defs", &mudstate.structs_htab);
	list_hashstat(player, "Component Defs", &mudstate.cdefs_htab);
	list_hashstat(player, "Instances", &mudstate.instance_htab);
	list_hashstat(player, "Instance Data", &mudstate.instdata_htab);
#ifdef USE_MAIL
	if (mudconf.have_mailer)
	    list_nhashstat(player, "Mail messages", &mudstate.mail_htab);
#endif
#ifdef USE_COMSYS
	if (mudconf.have_comsys)
	    list_hashstat(player, "Channel names", &mudstate.channel_htab);
#endif
}

static void list_textfiles(player)
    dbref player;
{
    int i;

    raw_notify(player, "Help File       Size Entries Deleted   Empty Lookups    Hits  Checks Longest");

    for (i = 0; i < mudstate.helpfiles; i++)
	list_hashstat(player, mudstate.hfiletab[i], &mudstate.hfile_hashes[i]);
}
			  

#ifndef MEMORY_BASED
/* These are from 'udb_cache.c'. */
extern time_t cs_ltime;
extern int cs_writes;		/* total writes */
extern int cs_reads;		/* total reads */
extern int cs_dbreads;		/* total read-throughs */
extern int cs_dbwrites;		/* total write-throughs */
extern int cs_dels;		/* total deletes */
extern int cs_checks;		/* total checks */
extern int cs_rhits;		/* total reads filled from cache */
extern int cs_ahits;		/* total reads filled active cache */
extern int cs_whits;		/* total writes to dirty cache */
extern int cs_fails;		/* attempts to grab nonexistent */
extern int cs_resets;		/* total cache resets */
extern int cs_syncs;		/* total cache syncs */
extern int cs_objects;		/* total cache size */

#endif /* MEMORY_BASED  */

#ifdef RADIX_COMPRESSION
extern int strings_compressed;	/* Total number of compressed strings */
extern int strings_decompressed;	/* Total number of decompressed
					 * strings 
					 */
extern int chars_in;		/* Total characters compressed */
extern int symbols_out;		/* Total symbols emitted */

#endif /* RADIX_COMPRESSION */

/* ---------------------------------------------------------------------------
 * list_db_stats: Get useful info from the DB layer about hash stats, etc.
 */

static void list_db_stats(player)
dbref player;
{
#ifdef MEMORY_BASED
	raw_notify(player, "Database is memory based.");
#else
	raw_notify(player,
	   tprintf("DB Cache Stats   Writes       Reads  (over %d seconds)",
		   time(0) - cs_ltime));
	raw_notify(player, tprintf("Calls      %12d%12d", cs_writes, cs_reads));
	raw_notify(player, tprintf("Cache Hits %12d%12d  (%d in active cache)",
				   cs_whits, cs_rhits, cs_ahits));
	raw_notify(player, tprintf("I/O        %12d%12d",
				   cs_dbwrites, cs_dbreads));
	raw_notify(player, tprintf("\nDeletes    %12d", cs_dels));
	raw_notify(player, tprintf("Checks     %12d", cs_checks));
	raw_notify(player, tprintf("Resets     %12d", cs_resets));
	raw_notify(player, tprintf("Syncs      %12d", cs_syncs));
	raw_notify(player, tprintf("Cache Size %12d", cs_objects));
#endif /* MEMORY_BASED */
#ifdef RADIX_COMPRESSION
	raw_notify(player, "Compression statistics:");
	raw_notify(player, tprintf("Strings compressed %d", strings_compressed));
	raw_notify(player, tprintf("Strings decompressed %d", strings_decompressed));
	raw_notify(player, tprintf("Compression ratio %d:%d", chars_in,
				   symbols_out + (symbols_out >> 1)));
#endif /* RADIX_COMPRESSION */
}

/* ---------------------------------------------------------------------------
 * list_process: List local resource usage stats of the mush process.
 * Adapted from code by Claudius@PythonMUCK,
 *     posted to the net by Howard/Dark_Lord.
 */

static void list_process(player)
dbref player;
{
	int pid, psize, maxfds;

#ifdef HAVE_GETRUSAGE
	struct rusage usage;
	int ixrss, idrss, isrss, curr, last, dur;

	getrusage(RUSAGE_SELF, &usage);
	/*
	 * Calculate memory use from the aggregate totals 
	 */

	curr = mudstate.mstat_curr;
	last = 1 - curr;
	dur = mudstate.mstat_secs[curr] - mudstate.mstat_secs[last];
	if (dur > 0) {
		ixrss = (mudstate.mstat_ixrss[curr] -
			 mudstate.mstat_ixrss[last]) / dur;
		idrss = (mudstate.mstat_idrss[curr] -
			 mudstate.mstat_idrss[last]) / dur;
		isrss = (mudstate.mstat_isrss[curr] -
			 mudstate.mstat_isrss[last]) / dur;
	} else {
		ixrss = 0;
		idrss = 0;
		isrss = 0;
	}
#endif

#ifdef HAVE_GETDTABLESIZE
	maxfds = getdtablesize();
#else
	maxfds = sysconf(_SC_OPEN_MAX);
#endif


	pid = getpid();
	psize = getpagesize();

	/*
	 * Go display everything 
	 */

	raw_notify(player,
		   tprintf("Process ID:  %10d        %10d bytes per page",
			   pid, psize));
#ifdef HAVE_GETRUSAGE
	raw_notify(player,
		   tprintf("Time used:   %10d user   %10d sys",
			   usage.ru_utime.tv_sec, usage.ru_stime.tv_sec));
/*
 * raw_notify(player,
 * * tprintf("Resident mem:%10d shared %10d private%10d stack",
 * * ixrss, idrss, isrss));
 */
	raw_notify(player,
		   tprintf("Integral mem:%10d shared %10d private%10d stack",
			   usage.ru_ixrss, usage.ru_idrss, usage.ru_isrss));
	raw_notify(player,
		   tprintf("Max res mem: %10d pages  %10d bytes",
			   usage.ru_maxrss, (usage.ru_maxrss * psize)));
	raw_notify(player,
	       tprintf("Page faults: %10d hard   %10d soft   %10d swapouts",
		       usage.ru_majflt, usage.ru_minflt, usage.ru_nswap));
	raw_notify(player,
		   tprintf("Disk I/O:    %10d reads  %10d writes",
			   usage.ru_inblock, usage.ru_oublock));
	raw_notify(player,
		   tprintf("Network I/O: %10d in     %10d out",
			   usage.ru_msgrcv, usage.ru_msgsnd));
	raw_notify(player,
		   tprintf("Context swi: %10d vol    %10d forced %10d sigs",
		       usage.ru_nvcsw, usage.ru_nivcsw, usage.ru_nsignals));
	raw_notify(player,
		   tprintf("Descs avail: %10d", maxfds));
#endif
}
/* ---------------------------------------------------------------------------
 * do_list: List information stored in internal structures.
 */

#define	LIST_ATTRIBUTES	1
#define	LIST_COMMANDS	2
#define	LIST_COSTS	3
#define	LIST_FLAGS	4
#define	LIST_FUNCTIONS	5
#define	LIST_GLOBALS	6
#define	LIST_ALLOCATOR	7
#define	LIST_LOGGING	8
#define	LIST_DF_FLAGS	9
#define	LIST_PERMS	10
#define	LIST_ATTRPERMS	11
#define	LIST_OPTIONS	12
#define	LIST_HASHSTATS	13
#define	LIST_BUFTRACE	14
#define	LIST_CONF_PERMS	15
#define	LIST_SITEINFO	16
#define	LIST_POWERS	17
#define	LIST_SWITCHES	18
#define	LIST_VATTRS	19
#define	LIST_DB_STATS	20	/* GAC 4/6/92 */
#define	LIST_PROCESS	21
#define	LIST_BADNAMES	22
#define LIST_CACHEOBJS	23
#define LIST_TEXTFILES  24
/* *INDENT-OFF* */

NAMETAB list_names[] = {
{(char *)"allocations",		2,	CA_WIZARD,	LIST_ALLOCATOR},
{(char *)"attr_permissions",	5,	CA_WIZARD,	LIST_ATTRPERMS},
{(char *)"attributes",		2,	CA_PUBLIC,	LIST_ATTRIBUTES},
{(char *)"bad_names",		2,	CA_WIZARD,	LIST_BADNAMES},
{(char *)"buffers",		2,	CA_WIZARD,	LIST_BUFTRACE},
{(char *)"cache",		2,	CA_WIZARD,	LIST_CACHEOBJS},
{(char *)"commands",		3,	CA_PUBLIC,	LIST_COMMANDS},
{(char *)"config_permissions",	3,	CA_GOD,		LIST_CONF_PERMS},
{(char *)"costs",		3,	CA_PUBLIC,	LIST_COSTS},
{(char *)"db_stats",		2,	CA_WIZARD,	LIST_DB_STATS},
{(char *)"default_flags",	1,	CA_PUBLIC,	LIST_DF_FLAGS},
{(char *)"flags",		2,	CA_PUBLIC,	LIST_FLAGS},
{(char *)"functions",		2,	CA_PUBLIC,	LIST_FUNCTIONS},
{(char *)"globals",		1,	CA_WIZARD,	LIST_GLOBALS},
{(char *)"hashstats",		1,	CA_WIZARD,	LIST_HASHSTATS},
{(char *)"logging",		1,	CA_GOD,		LIST_LOGGING},
{(char *)"options",		1,	CA_PUBLIC,	LIST_OPTIONS},
{(char *)"permissions",		2,	CA_WIZARD,	LIST_PERMS},
{(char *)"powers",		2,	CA_WIZARD,	LIST_POWERS},
{(char *)"process",		2,	CA_WIZARD,	LIST_PROCESS},
{(char *)"site_information",	2,	CA_WIZARD,	LIST_SITEINFO},
{(char *)"switches",		2,	CA_PUBLIC,	LIST_SWITCHES},
{(char *)"textfiles",		1,	CA_WIZARD,	LIST_TEXTFILES},
{(char *)"user_attributes",	1,	CA_WIZARD,	LIST_VATTRS},
{ NULL,				0,	0,		0}};

/* *INDENT-ON* */

extern NAMETAB enable_names[];
extern NAMETAB logoptions_nametab[];
extern NAMETAB logdata_nametab[];

void do_list(player, cause, extra, arg)
dbref player, cause;
int extra;
char *arg;
{
	int flagvalue;

	flagvalue = search_nametab(player, list_names, arg);
	switch (flagvalue) {
	case LIST_ALLOCATOR:
		list_bufstats(player);
		break;
	case LIST_BUFTRACE:
		list_buftrace(player);
		break;
	case LIST_ATTRIBUTES:
		list_attrtable(player);
		break;
	case LIST_COMMANDS:
		list_cmdtable(player);
		break;
	case LIST_SWITCHES:
		list_cmdswitches(player);
		break;
	case LIST_COSTS:
		list_costs(player);
		break;
	case LIST_OPTIONS:
		list_options(player);
		break;
	case LIST_HASHSTATS:
		list_hashstats(player);
		break;
	case LIST_SITEINFO:
		list_siteinfo(player);
		break;
	case LIST_FLAGS:
		display_flagtab(player);
		break;
	case LIST_FUNCTIONS:
		list_functable(player);
		break;
	case LIST_GLOBALS:
		interp_nametab(player, enable_names, mudconf.control_flags,
			    (char *)"Global parameters:", (char *)"enabled",
			       (char *)"disabled");
		break;
	case LIST_DF_FLAGS:
		list_df_flags(player);
		break;
	case LIST_PERMS:
		list_cmdaccess(player);
		break;
	case LIST_CONF_PERMS:
		list_cf_access(player);
		break;
	case LIST_POWERS:
		display_powertab(player);
		break;
	case LIST_ATTRPERMS:
		list_attraccess(player);
		break;
	case LIST_VATTRS:
		list_vattrs(player);
		break;
	case LIST_LOGGING:
		interp_nametab(player, logoptions_nametab, mudconf.log_options,
			       (char *)"Events Logged:", (char *)"enabled",
			       (char *)"disabled");
		interp_nametab(player, logdata_nametab, mudconf.log_info,
			       (char *)"Information Logged:", (char *)"yes",
			       (char *)"no");
		break;
	case LIST_DB_STATS:
		list_db_stats(player);
		break;
	case LIST_PROCESS:
		list_process(player);
		break;
	case LIST_BADNAMES:
		badname_list(player, "Disallowed names:");
		break;
	case LIST_CACHEOBJS:
#ifndef MEMORY_BASED
		list_cached_objs(player);
#else
		raw_notify(player, "No cached objects.");
#endif /* MEMORY_BASED */
		break;
	case LIST_TEXTFILES:
		list_textfiles(player);
		break;
	default:
		display_nametab(player, list_names,
				(char *)"Unknown option.  Use one of:", 1);
	}
}

