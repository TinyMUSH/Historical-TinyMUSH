/* db_msql.c */

/* Implements accessing an mSQL 2.x database. */

#include "copyright.h"
#include "autoconf.h"
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "msql.h"

/* See db_sql.h for details of what each of these functions do. */


/* Because we cannot trap error codes in mSQL, just error messages, we have
 * to compare against error messages to find out what really happened with
 * something. This particular error message is SERVER_GONE_ERROR in
 * mSQL's errmsg.h -- this needs to change if that API definition changes.
 */

#define MSQL_SERVER_GONE_ERROR "MSQL server has gone away"

/* Number of times to retry a connection if we fail in the middle of
 * a query.
 */

#define MSQL_RETRY_TIMES 3

int sql_init()
{
    int result;

    /* If we are already connected, drop and retry the connection, in
     * case for some reason the server went away.
     */

    if (mudstate.sql_socket != -1)
	msqlClose(mudstate.sql_socket);

    /* Make sure we have valid config options. */

    if (!mudconf.sql_host || !*mudconf.sql_host)
	return -1;
    if (!mudconf.sql_db || !*mudconf.sql_db)
	return -1;

    /* Try to connect to the database host. If we have specified
     * localhost, use the Unix domain socket instead.
     */

    if (!strcmp(mudconf.sql_host, (char *) "localhost"))
	result = msqlConnect(NULL);
    else
	result = msqlConnect(mudconf.sql_host);
    if (result == -1) {
	STARTLOG(LOG_ALWAYS, "SQL", "CONN")
	    if (msqlErrMsg) {
		log_text((char *) "Failed connect to SQL server: ");
		log_text(msqlErrMsg);
	    }
	ENDLOG
	return -1;
    }

    STARTLOG(LOG_ALWAYS, "SQL", "CONN")
	log_text((char *) "Connected to SQL server.");
    ENDLOG
    mudstate.sql_socket = result;

    /* Select the database we want. If we can't get it, disconnect. */

    if (msqlSelectDB(mudstate.sql_socket, mudconf.sql_db) == -1) {
	STARTLOG(LOG_ALWAYS, "SQL", "CONN")
	    log_text((char *) "Failed db select: ");
	    log_text(msqlErrMsg);
	ENDLOG
	msqlClose(mudstate.sql_socket);
	return -1;
    }

    STARTLOG(LOG_ALWAYS, "SQL", "CONN")
	log_text((char *) "SQL database selected.");
    ENDLOG
    return (mudstate.sql_socket);
}

int sql_query(player, q_string, buff, bufc, row_delim, field_delim)
    dbref player;
    char *q_string;
    char *buff;
    char **bufc;
    char row_delim, field_delim;
{
    m_result *qres;
    m_row row_p;
    int got_rows, got_fields;
    int i, j;
    int retries;

    /* If we have no connection, this is an error generating a #-1.
     * Notify the player, too, and set the return code.
     */
    if (mudstate.sql_socket == -1) {
	notify(player, "No SQL database connection.");
	safe_str("#-1", buff, bufc);
	return -1;
    }
    if (!q_string || !*q_string)
	return 0;

    /* Send the query. */

    got_rows = msqlQuery(mudstate.sql_socket, q_string);
    if ((got_rows == -1) && !strcmp(msqlErrMsg, MSQL_SERVER_GONE_ERROR)) {

	/* We got this error because the server died unexpectedly
	 * and it shouldn't have. Try repeatedly to reconnect before
	 * giving up and failing. This induces a few seconds of lag,
	 * depending on number of retries; we put in the sleep() here
	 * to see if waiting a little bit helps.
	 */

	retries = 0;
	while ((retries < MSQL_RETRY_TIMES) &&
	       (mudstate.sql_socket == -1)) {
	    sleep(1);
	    sql_init();
	    retries++;
	}

	if (mudstate.sql_socket != -1)
	    got_rows = msqlQuery(mudstate.sql_socket, q_string);
    }
    if (got_rows == -1) {
	notify(player, msqlErrMsg);
	safe_str("#-1", buff, bufc);
        return -1;
    }

    /* A null store means that this wasn't a SELECT */

    qres = msqlStoreResult();
    if (!qres)
	return 0;

    /* Check to make sure we got rows back. */

    got_rows = msqlNumRows(qres);
    if (got_rows == 0) {
	return 0;
    }

    /* Construct properly-delimited data. */

    for (i = 0; i < got_rows; i++) {
	if (i > 0)
	    safe_chr(row_delim, buff, bufc);
	row_p = msqlFetchRow(qres);
	if (row_p) {
	    got_fields = msqlNumFields(qres);
	    for (j = 0; j < got_fields; j++) {
		if (j > 0)
		    safe_chr(field_delim, buff, bufc);
		if (row_p[j] && *row_p[j])
		    safe_str(row_p[j], buff, bufc);
	    }
	}
    }

    msqlFreeResult(qres);
    return 0;
}


void sql_shutdown()
{
    if (mudstate.sql_socket == -1)
	return;
    msqlClose(mudstate.sql_socket);
    mudstate.sql_socket = -1;
}
