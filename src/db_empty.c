/* db_empty.c */

/* Placeholder code if we're not using an external SQL database. */

#include "copyright.h"
#include "autoconf.h"
#include "mudconf.h"
#include "config.h"
#include "externs.h"

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
