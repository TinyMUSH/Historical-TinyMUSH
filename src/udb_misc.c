/* udb_misc.c - Misc support routines for unter style error management. */
/* $Id$ */

/* 
 * Stolen from mjr.
 * 
 * Modded to scribble on stderr, for now.
 * 
 * Andrew Molitor, amolitor@eagle.wesleyan.edu
 */

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

#include "udb_defs.h"	/* required by code */

/*
 * Log database errors 
 */

void log_db_err(obj, attr, txt)
int obj, attr;
const char *txt;
{
	STARTLOG(LOG_ALWAYS, "DBM", "ERROR")
	log_printf("Could not %s object #%d", txt, obj);
	if (attr != NOTHING) {
		log_printf(" attr #%d", attr);
	}
	ENDLOG
}

/*
 * print a series of warnings - do not exit
 */
/*
 * VARARGS 
 */
#ifdef STDC_HEADERS
void logf(char *p,...)
#else
void logf(va_alist)
va_dcl

#endif

{
	va_list ap;

#ifdef STDC_HEADERS
	va_start(ap, p);
#else
	char *p;

	va_start(ap);
	p = va_arg(ap, char *);

#endif
	while (1) {
		if (p == (char *)0)
			break;

		if (p == (char *)-1)
			p = (char *) sys_errlist[errno];

		(void)fprintf(stderr, "%s", p);
		p = va_arg(ap, char *);
	}
	va_end(ap);
	(void)fflush(stderr);
}

/*
 * print a series of warnings - exit
 */
/*
 * VARARGS 
 */
#ifdef STDC_HEADERS
void fatal(char *p,...)
#else
void fatal(va_alist)
va_dcl

#endif
{
	va_list ap;

#ifdef STDC_HEADERS
	va_start(ap, p);
#else
	char *p;

	va_start(ap);
	p = va_arg(ap, char *);

#endif
	while (1) {
		if (p == (char *)0)
			break;

		if (p == (char *)-1)
			p = (char *) sys_errlist[errno];

		(void)fprintf(stderr, "%s", p);
		p = va_arg(ap, char *);
	}
	va_end(ap);
	(void)fflush(stderr);
	exit(1);
}
