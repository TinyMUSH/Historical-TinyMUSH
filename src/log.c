/* log.c - logging routines */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"

#include <sys/types.h>

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"
#include "alloc.h"
#include "htab.h"
#include "ansi.h"

FILE *mainlog_fp;
static FILE *log_fp = NULL;

#ifndef STANDALONE

/* *INDENT-OFF* */

NAMETAB logdata_nametab[] = {
{(char *)"flags",		1,	0,	LOGOPT_FLAGS},
{(char *)"location",		1,	0,	LOGOPT_LOC},
{(char *)"owner",		1,	0,	LOGOPT_OWNER},
{(char *)"timestamp",		1,	0,	LOGOPT_TIMESTAMP},
{ NULL,				0,	0,	0}};

NAMETAB logoptions_nametab[] = {
{(char *)"accounting",		2,	0,	LOG_ACCOUNTING},
{(char *)"all_commands",	2,	0,	LOG_ALLCOMMANDS},
{(char *)"bad_commands",	2,	0,	LOG_BADCOMMANDS},
{(char *)"buffer_alloc",	3,	0,	LOG_ALLOCATE},
{(char *)"bugs",		3,	0,	LOG_BUGS},
{(char *)"checkpoints",		2,	0,	LOG_DBSAVES},
{(char *)"config_changes",	2,	0,	LOG_CONFIGMODS},
{(char *)"create",		2,	0,	LOG_PCREATES},
{(char *)"keyboard_commands",	2,	0,	LOG_KBCOMMANDS},
{(char *)"killing",		1,	0,	LOG_KILLS},
{(char *)"logins",		1,	0,	LOG_LOGIN},
{(char *)"network",		1,	0,	LOG_NET},
{(char *)"problems",		1,	0,	LOG_PROBLEMS},
{(char *)"security",		2,	0,	LOG_SECURITY},
{(char *)"shouts",		2,	0,	LOG_SHOUTS},
{(char *)"startup",		2,	0,	LOG_STARTUP},
{(char *)"suspect_commands",	2,	0,	LOG_SUSPECTCMDS},
{(char *)"time_usage",		1,	0,	LOG_TIMEUSE},
{(char *)"wizard",		1,	0,	LOG_WIZARD},
{ NULL,				0,	0,	0}};

LOGFILETAB logfds_table[] = {
{ LOG_ACCOUNTING,	NULL,		NULL},
{ LOG_ACCOUNTING,	NULL,		NULL},
{ LOG_ALLCOMMANDS,	NULL,		NULL},
{ LOG_BADCOMMANDS,	NULL,		NULL},
{ LOG_ALLOCATE,		NULL,		NULL},
{ LOG_BUGS,		NULL,		NULL},
{ LOG_DBSAVES,		NULL,		NULL},
{ LOG_CONFIGMODS,	NULL,		NULL},
{ LOG_PCREATES,		NULL,		NULL},
{ LOG_KBCOMMANDS,	NULL,		NULL},
{ LOG_KILLS,		NULL,		NULL},
{ LOG_LOGIN,		NULL,		NULL},
{ LOG_NET,		NULL,		NULL},
{ LOG_PROBLEMS,		NULL,		NULL},
{ LOG_SECURITY,		NULL,		NULL},
{ LOG_SHOUTS,		NULL,		NULL},
{ LOG_STARTUP,		NULL,		NULL},
{ LOG_SUSPECTCMDS,	NULL,		NULL},
{ LOG_TIMEUSE,		NULL,		NULL},
{ LOG_WIZARD,		NULL,		NULL},
{ 0,			NULL,		NULL}};

/* *INDENT-ON* */

#endif

char *strip_ansi(raw)
const char *raw;
{
	static char buf[LBUF_SIZE * 2];
	char *p = (char *)raw;
	char *q = buf;

	while (p && *p) {
		if (*p == ESC_CHAR) {
			/*
			 * Start of ANSI code. Skip to end. 
			 */
			while (*p && !isalpha(*p))
				p++;
			if (*p)
				p++;
		} else
			*q++ = *p++;
	}
	*q = '\0';
	return buf;
}

char *normal_to_white(raw)
const char *raw;
{
	static char buf[LBUF_SIZE * 2];
	char *p = (char *)raw;
	char *q = buf;


	while (p && *p) {
		if (*p == ESC_CHAR) {
			/* Start of ANSI code. */
			*q++ = *p++;	/* ESC CHAR */
			*q++ = *p++;	/* [ character. */
			if (*p == '0') {	/* ANSI_NORMAL */
				safe_known_str("0m", 2, buf, &q);
				safe_chr(ESC_CHAR, buf, &q);
				safe_known_str("[37m", 4, buf, &q);
				p += 2;
			}
		} else
			*q++ = *p++;
	}
	*q = '\0';
	return buf;
}


/* ---------------------------------------------------------------------------
 * logfile_init: Initialize the main logfile.
 */

void logfile_init(filename)
    char *filename;
{
    if (!filename) {
	fprintf(stderr, "No logfile name specified.\n");
	mainlog_fp = stderr;
	return;
    }

    mainlog_fp = fopen(filename, "w");
    if (!mainlog_fp) {
	fprintf(stderr, "Could not open logfile %s for writing.\n",
		filename);
	mainlog_fp = stderr;
	return;
    }

    setbuf(mainlog_fp, NULL);	/* unbuffered */
}


/* ---------------------------------------------------------------------------
 * start_log: see if it is OK to log something, and if so, start writing the
 * log entry.
 */

int start_log(primary, secondary, key)
const char *primary, *secondary;
int key;
{
	struct tm *tp;
	time_t now;
	LOGFILETAB *lp;

#ifndef STANDALONE
	static int last_key = 0;

	if (mudconf.log_diversion & key) {
	    if (key != last_key) { /* Try to save ourselves some lookups */
		last_key = key;
		for (lp = logfds_table; lp->log_flag; lp++) {
		    /* Though keys can be OR'd, use the first one matched */
		    if (lp->log_flag & key) {
			log_fp = lp->fileptr;
			break;
		    }
		}
		if (!log_fp)
		    log_fp = mainlog_fp;
	    }
	} else {
	    last_key = key;
	    log_fp = mainlog_fp;
	}
#endif /* ! STANDALONE */

	mudstate.logging++;
	switch (mudstate.logging) {
	case 1:
	case 2:

		/* Format the timestamp */

		if ((mudconf.log_info & LOGOPT_TIMESTAMP) != 0) {
			time((time_t *) (&now));
			tp = localtime((time_t *) (&now));
			sprintf(mudstate.buffer, "%02d%d%d%d%d.%d%d%d%d%d%d ",
				(tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
				(((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
				(tp->tm_mday % 10),
				(tp->tm_hour / 10), (tp->tm_hour % 10),
				(tp->tm_min / 10), (tp->tm_min % 10),
				(tp->tm_sec / 10), (tp->tm_sec % 10));
		} else {
			mudstate.buffer[0] = '\0';
		}
#ifndef STANDALONE
		/* Write the header to the log */

		if (secondary && *secondary)
			fprintf(log_fp, "%s%s %3s/%-5s: ", mudstate.buffer,
				mudconf.mud_name, primary, secondary);
		else
			fprintf(log_fp, "%s%s %-9s: ", mudstate.buffer,
				mudconf.mud_name, primary);
#endif
		/* If a recursive call, log it and return indicating no log */

		if (mudstate.logging == 1)
			return 1;
		fprintf(mainlog_fp, "Recursive logging request.\r\n");
	default:
		mudstate.logging--;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * end_log: Finish up writing a log entry
 */

void NDECL(end_log)
{
	fprintf(log_fp, "\n");
	fflush(log_fp);
	mudstate.logging--;
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void log_perror(primary, secondary, extra, failing_object)
const char *primary, *secondary, *extra, *failing_object;
{
	start_log(primary, secondary, LOG_ALWAYS);
	if (extra && *extra) {
		log_string((char *)"(");
		log_text((char *)extra);
		log_string((char *)") ");
	}
	log_text((char *)failing_object);
	log_string((char *)": ");
	log_string(strerror(errno));
	end_log();
}

/* ---------------------------------------------------------------------------
 * log_text, log_number: Write text or number to the log file.
 */

void log_string(text)
char *text;
{
	fprintf(log_fp, "%s", text);
}

void log_text(text)
char *text;
{
	fprintf(log_fp, "%s", strip_ansi(text));
}

void log_number(num)
int num;
{
	fprintf(log_fp, "%d", num);
}

/* ---------------------------------------------------------------------------
 * log_name: write the name, db number, and flags of an object to the log.
 * If the object does not own itself, append the name, db number, and flags
 * of the owner.
 */

void log_name(target)
dbref target;
{
#ifndef STANDALONE
	char *tp;

	if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
		tp = unparse_object((dbref) GOD, target, 0);
	else
		tp = unparse_object_numonly(target);
	fprintf(log_fp, "%s", strip_ansi(tp));
	free_lbuf(tp);
	if (((mudconf.log_info & LOGOPT_OWNER) != 0) &&
	    (target != Owner(target))) {
		if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
			tp = unparse_object((dbref) GOD, Owner(target), 0);
		else
			tp = unparse_object_numonly(Owner(target));
		fprintf(log_fp, "[%s]", strip_ansi(tp));
		free_lbuf(tp);
	}
#else
	fprintf(stderr, "%s(#%d)", Name(target), target);
#endif
	return;
}

/* ---------------------------------------------------------------------------
 * log_name_and_loc: Log both the name and location of an object
 */

void log_name_and_loc(player)
dbref player;
{
	log_name(player);
	if ((mudconf.log_info & LOGOPT_LOC) && Has_location(player)) {
		log_text((char *)" in ");
		log_name(Location(player));
	}
	return;
}

char *OBJTYP(thing)
dbref thing;
{
	if (!Good_obj(thing)) {
		return (char *)"??OUT-OF-RANGE??";
	}
	switch (Typeof(thing)) {
	case TYPE_PLAYER:
		return (char *)"PLAYER";
	case TYPE_THING:
		return (char *)"THING";
	case TYPE_ROOM:
		return (char *)"ROOM";
	case TYPE_EXIT:
		return (char *)"EXIT";
	case TYPE_GARBAGE:
		return (char *)"GARBAGE";
	default:
		return (char *)"??ILLEGAL??";
	}
}

void log_type_and_name(thing)
dbref thing;
{
	char nbuf[16];

	log_text(OBJTYP(thing));
	sprintf(nbuf, " #%d(", thing);
	log_text(nbuf);
	if (Good_obj(thing))
		log_text(Name(thing));
	log_text((char *)")");
	return;
}
