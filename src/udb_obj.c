/* udb_obj.c - Binary object handling gear. Shit simple. */
/* $Id$ */

/* 
 * Andrew Molitor, amolitor@nmsu.edu
 * 
 * 1992
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "udb.h"	/* required by code */

Attr *attrfromFILE(buff)
char *buff;
{
	return((Attr *)strdup(buff));
}


int attrtoFILE(atr, buff)
Attr *atr;
char *buff;
{
	strcpy(buff, (char *)atr);
	return 0;
}
