/* version.c - version information */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "patchlevel.h"	/* required by code */

/*
 * TinyMUSH 3.0
 */

/*
 * 2.0
 * All known bugs fixed with disk-based.  Played with gdbm, it
 * sucked.  Now using bsd 4.4 hash stuff.
 */

/*
 * 1.12
 * * All known bugs fixed after several days of debugging 1.10/1.11.
 * * Much string-handling braindeath patched, but needs a big overhaul,
 * * really.   GAC 2/10/91
 */

/*
 * 1.11
 * * Fixes for 1.10.  (@name didn't call do_name, etc.)
 * * Added dexamine (debugging examine, dumps the struct, lots of info.)
 */

/*
 * 1.10
 * * Finally got db2newdb working well enough to run from the big (30000
 * * object) db with ATR_KEY and ATR_NAME defined.   GAC 2/3/91
 */

/*
 * TinyMUSH version.c file.  Add a comment here any time you've made a
 * * big enough revision to increment the TinyMUSH version #.
 */

void do_version(player, cause, extra)
dbref player, cause;
int extra;
{
	notify(player, mudstate.version);
	notify(player, tprintf("Build date: %s", MUSH_BUILD_DATE));
	if (Wizard(player)) {
		notify(player, tprintf("Build info: %s", mudstate.buildinfo));
	}
	if (mudstate.modloaded[0]) {
	    notify(player, tprintf("Modules loaded: %s", mudstate.modloaded));
	}
}

void NDECL(init_version)
{

#if PATCHLEVEL > 0
    mudstate.version =
	XSTRDUP(tprintf("TinyMUSH version %s %s %d #%s [%s]",
			MUSH_VERSION,
			((MUSH_RELEASE_STATUS == 0) ? "patchlevel" :
			 ((MUSH_RELEASE_STATUS == 1) ? "beta" : "alpha")),
			PATCHLEVEL, MUSH_BUILD_NUM, MUSH_RELEASE_DATE),
		"init_version");
    mudstate.short_ver =
	XSTRDUP(tprintf("TinyMUSH %s.%c%d",
			MUSH_VERSION,
			((MUSH_RELEASE_STATUS == 0) ? 'p' :
			 ((MUSH_RELEASE_STATUS == 1) ? 'b' : 'a')),
			PATCHLEVEL), "init_version");
#else
    mudstate.version =
	XSTRDUP(tprintf("TinyMUSH version %s #%s [%s]",
			MUSH_VERSION, MUSH_BUILD_NUM, MUSH_RELEASE_DATE),
		"init_version");
    mudstate.short_ver =
	XSTRDUP(tprintf("TinyMUSH %s", MUSH_VERSION), "init_version");
#endif /* PATCHLEVEL > 0 */

	mudstate.buildinfo =
		XSTRDUP(tprintf("%s\n            %s %s",
				MUSH_CONFIGURE_CMD,
				MUSH_BUILD_COMPILER, MUSH_BUILD_CFLAGS),
			"init_version");
	STARTLOG(LOG_ALWAYS, "INI", "START")
		log_printf("Starting: %s", mudstate.version);
	ENDLOG
	STARTLOG(LOG_ALWAYS, "INI", "START")
		log_printf("Build date: %s", MUSH_BUILD_DATE);
	ENDLOG
	STARTLOG(LOG_ALWAYS, "INI", "START")
		log_printf("Build info: %s", mudstate.buildinfo);
	ENDLOG
}
