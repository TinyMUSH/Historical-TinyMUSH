/* help.c - commands for giving help */
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

#include "help.h"	/* required by code */

/* Pointers to this struct is what gets stored in the help_htab's */
struct help_entry {
	int pos;		/* Position, copied from help_indx */
	char original;		/* 1 for the longest name for a topic. 0 for
				 * abbreviations 
				 */
	char *key;		/* The key this is stored under. */
};

int helpindex_read(htab, filename)
HASHTAB *htab;
char *filename;
{
	help_indx entry;
	char *p;
	int count;
	FILE *fp;
	struct help_entry *htab_entry;

	/* Let's clean out our hash table, before we throw it away. */
	for (htab_entry = (struct help_entry *)hash_firstentry(htab);
	     htab_entry;
	     htab_entry = (struct help_entry *)hash_nextentry(htab)) {
		XFREE(htab_entry->key, "helpindex_read.topic0");
		XFREE(htab_entry, "helpindex_read.hent0");
	}

	hashflush(htab, 0);

	if ((fp = tf_fopen(filename, O_RDONLY)) == NULL) {
		STARTLOG(LOG_PROBLEMS, "HLP", "RINDX")
		log_printf("Can't open %s for reading.", filename);
		ENDLOG
		return -1;
	}
	count = 0;
	while ((fread((char *)&entry, sizeof(help_indx), 1, fp)) == 1) {

		/* Lowercase the entry and add all leftmost substrings.
		 * Substrings already added will be rejected by hashadd. 
		 */
		for (p = entry.topic; *p; p++)
			*p = ToLower(*p);

		htab_entry = (struct help_entry *)XMALLOC(sizeof(struct help_entry),
							  "helpindex_read.hent1");

		htab_entry->pos = entry.pos;
		htab_entry->original = 1;	/* First is the longest */
		htab_entry->key = XSTRDUP(entry.topic, "helpindex_read.topic1");

		while (p > entry.topic) {
			p--;
			if (!isspace(*p)) {
				if (hashadd(entry.topic, (int *)htab_entry, htab) == 0) {
					count++;
				} else {
					/* It didn't make it into the hash
					 * table, so neither will any of the
					 * left substrings
					 */
					break;
				}
				*p = '\0';
				htab_entry = (struct help_entry *) XMALLOC(sizeof(struct help_entry),
									   "helpindex_read.hent");
				htab_entry->pos = entry.pos;
				htab_entry->original = 0;
				htab_entry->key = XSTRDUP(entry.topic, "helpindex_read.topic");
			} else {
				/* cut off the trailing space and try again */
				htab_entry->original = 0;
				htab_entry->key[strlen(htab_entry->key)] = '\0';
			}
		}
		XFREE(htab_entry->key, "helpindex_read.topic");
		XFREE(htab_entry, "helpindex_read.hent");
	}
	tf_fclose(fp);
	hashreset(htab);
	return count;
}

void helpindex_load(player)
dbref player;
{
    int i;
    char buf[SBUF_SIZE + 8];

    if (!mudstate.hfiletab) {
	if ((player != NOTHING) && !Quiet(player))
	    notify(player, "No indexed files have been configured.");
	return;
    }

    for (i = 0; i < mudstate.helpfiles; i++) {
	sprintf(buf, "%s.indx", mudstate.hfiletab[i]);
	helpindex_read(&mudstate.hfile_hashes[i], buf);
    }

    if ((player != NOTHING) && !Quiet(player))
	notify(player, "Indexed file cache updated.");
}


void NDECL(helpindex_init)
{
    /* We do not need to do hashinits here, as this will already have
     * been done by the add_helpfile() calls.
     */

    helpindex_load(NOTHING);
}

void help_write(player, topic, htab, filename, eval)
dbref player;
char *topic, *filename;
HASHTAB *htab;
int eval;
{
	FILE *fp;
	char *p, *line, *bp, *str, *result;
	int offset;
	struct help_entry *htab_entry;
	char matched;
	char *topic_list, *buffp;

	if (*topic == '\0')
		topic = (char *)"help";
	else
		for (p = topic; *p; p++)
			*p = ToLower(*p);
	htab_entry = (struct help_entry *)hashfind(topic, htab);
	if (htab_entry) {
		offset = htab_entry->pos;
	} else {
		matched = 0;
		for (htab_entry = (struct help_entry *)hash_firstentry(htab);
		     htab_entry != NULL;
		   htab_entry = (struct help_entry *)hash_nextentry(htab)) {
			if (htab_entry->original &&
			    quick_wild(topic, htab_entry->key)) {
				if (matched == 0) {
					matched = 1;
					topic_list = alloc_lbuf("help_write");
					buffp = topic_list;
				}
				safe_str(htab_entry->key, topic_list, &buffp);
				safe_chr(' ', topic_list, &buffp);
				safe_chr(' ', topic_list, &buffp);
			}
		}
		if (matched == 0)
			notify(player, tprintf("No entry for '%s'.", topic));
		else {
			notify(player, tprintf("Here are the entries which match '%s':", topic));
			*buffp = '\0';
			notify(player, topic_list);
			free_lbuf(topic_list);
		}
		return;
	}
	if ((fp = tf_fopen(filename, O_RDONLY)) == NULL) {
		notify(player,
		       "Sorry, that function is temporarily unavailable.");
		STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
		log_printf("Can't open %s for reading.", filename);
		ENDLOG
		return;
	}
	if (fseek(fp, offset, 0) < 0L) {
		notify(player,
		       "Sorry, that function is temporarily unavailable.");
		STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
		log_printf("Seek error in file %s.", filename);
		ENDLOG
		tf_fclose(fp);
		return;
	}
	line = alloc_lbuf("help_write");
	result = alloc_lbuf("help_write.2");
	for (;;) {
		if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
			break;
		if (line[0] == '&')
			break;
		for (p = line; *p != '\0'; p++)
			if (*p == '\n')
				*p = '\0';
		if (eval) {
			str = line;
			bp = result;
			exec(result, &bp, 0, player, player,
			     EV_NO_COMPRESS | EV_FIGNORE | EV_EVAL, &str,
			     (char **)NULL, 0);
			*bp = '\0';
			notify(player, result);
		} else
			notify(player, line);
	}
	tf_fclose(fp);
	free_lbuf(line);
	free_lbuf(result);
}

/* ---------------------------------------------------------------------------
 * do_help: display information from new-format news and help files
 */

void do_help(player, cause, key, message)
dbref player, cause;
int key;
char *message;
{
    char tbuf[SBUF_SIZE + 8];
    int hf_num;

    hf_num = key & ~HELP_RAWHELP;

    if (hf_num >= mudstate.helpfiles) {
	STARTLOG(LOG_BUGS, "BUG", "HELP")
	    log_printf("Unknown help file number: %d", hf_num);
	ENDLOG
	notify(player, "No such indexed file found.");
	return;
    }

    sprintf(tbuf, "%s.txt", mudstate.hfiletab[hf_num]);
    help_write(player, message, &mudstate.hfile_hashes[hf_num], tbuf,
	       (key & HELP_RAWHELP) ? 0 : 1);
}
