/* version.c - version information */
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
 * TinyCWRU version.c file.  Add a comment here any time you've made a
 * * big enough revision to increment the TinyCWRU version #.
 */

void do_version(player, cause, extra)
dbref player, cause;
int extra;
{
	char *buff;

	notify(player, mudstate.version);
	buff = alloc_mbuf("do_version");
	sprintf(buff, "Build date: %s", MUSH_BUILD_DATE);
	notify(player, buff);
	free_mbuf(buff);
}

void NDECL(init_version)
{
#ifdef BETA
#if PATCHLEVEL > 0
	sprintf(mudstate.version, "TinyMUSH Beta version %s patchlevel %d #%s",
		MUSH_VERSION, PATCHLEVEL, MUSH_BUILD_NUM);
	sprintf(mudstate.short_ver, "TinyMUSH Beta %s.p%d",
		MUSH_VERSION, PATCHLEVEL);
#else
	sprintf(mudstate.version, "TinyMUSH Beta version %s #%s",
		MUSH_VERSION, MUSH_BUILD_NUM);
	sprintf(mudstate.short_ver, "TinyMUSH Beta %s",
		MUSH_VERSION);
#endif /*
        * PATCHLEVEL 
        */
#else /*
       * not BETA 
       */
#if PATCHLEVEL > 0
	sprintf(mudstate.version, "TinyMUSH version %s patchlevel %d #%s [%s]",
		MUSH_VERSION, PATCHLEVEL, MUSH_BUILD_NUM, MUSH_RELEASE_DATE);
	sprintf(mudstate.short_ver, "TinyMUSH %s.p%d",
		MUSH_VERSION, PATCHLEVEL);
#else
	sprintf(mudstate.version, "TinyMUSH version %s #%s [%s]",
		MUSH_VERSION, MUSH_BUILD_NUM, MUSH_RELEASE_DATE);
	sprintf(mudstate.short_ver, "TinyMUSH %s",
		MUSH_VERSION);
#endif /*
        * PATCHLEVEL 
        */
#endif /*
        * BETA 
        */
	STARTLOG(LOG_ALWAYS, "INI", "START")
		log_printf("Starting: %s", mudstate.version);
	ENDLOG
	STARTLOG(LOG_ALWAYS, "INI", "START")
		log_printf("Build date: %s", MUSH_BUILD_DATE);
	ENDLOG
}
