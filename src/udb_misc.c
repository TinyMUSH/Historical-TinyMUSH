/*
 * Misc support routines for unter style error management.
 * Stolen from mjr.
 * 
 * Andrew Molitor, amolitor@eagle.wesleyan.edu
 * 
 * $Id$
 */

#include "autoconf.h"
#include "udb_defs.h"
#include "externs.h"

/*
 * Log database errors 
 */

void log_db_err(obj, attr, txt)
int obj, attr;
const char *txt;
{
	STARTLOG(LOG_ALWAYS, "DBM", "ERROR")
		log_text((char *)"Could not ");
	log_text((char *)txt);
	log_text((char *)" object #");
	log_number(obj);
	if (attr != NOTHING) {
		log_text((char *)" attr #");
		log_number(attr);
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

		(void)fprintf(mainlog_fp, "%s", p);
		p = va_arg(ap, char *);
	}
	va_end(ap);
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

		(void)fprintf(mainlog_fp, "%s", p);
		p = va_arg(ap, char *);
	}
	va_end(ap);
	exit(1);
}
