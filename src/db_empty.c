/* db_empty.c - Placeholder code if not using an external SQL database. */
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

/* See db_sql.h for details of what each of these functions do. */

int sql_init()
{
    return -1;
}

int sql_query(player, q_string, buff, bufc, row_delim, field_delim)
    dbref player;
    char *q_string;
    char *buff;
    char **bufc;
    char row_delim, field_delim;
{
    notify(player, "No external SQL database connectivity is configured.");
    if (buff)
	safe_str("#-1", buff, bufc);
    return -1;
}


void sql_shutdown()
{
    mudstate.sql_socket = -1;
}
