/* hello.c - demonstration module */
/* $Id$ */

#include "../api.h"

#define MOD_HELLO_HELLO_INFORMAL	1

/* --------------------------------------------------------------------------
 * Conf table.
 */

struct mod_hello_confstorage {
	int show_name;
	char *hello_string;
	int hello_times;
} mod_hello_config;

CONF mod_hello_conftable[] = {
{(char *)"hello_shows_name",		cf_bool,	CA_GOD,		CA_PUBLIC,	(int *)&mod_hello_config.show_name,	(long)"Greet players by name"},
{(char *)"hello_string",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mod_hello_config.hello_string,	MBUF_SIZE},
{(char *)"hello_times",			cf_int,		CA_GOD,		CA_PUBLIC,	(int *)&mod_hello_config.hello_times,	5},
{ NULL,					NULL,		0,		0,		NULL,				0}};

/* --------------------------------------------------------------------------
 * Database.
 */

typedef struct mod_hello_dbobj MOD_HELLO_OBJ;
struct mod_hello_dbobj {
    int greeted;
    int foofed;
};

MOD_HELLO_OBJ *mod_hello_db = NULL;

#define OBJ_INIT_MODULE(x) \
    mod_hello_db[x].greeted = 0; \
    mod_hello_db[x].foofed = 0;

void mod_hello_db_grow(newsize, newtop)
    int newsize, newtop;
{
    DB_GROW_MODULE(mod_hello_db, newsize, newtop, MOD_HELLO_OBJ);
}

/* --------------------------------------------------------------------------
 * Handlers.
 */

int mod_hello_process_command(player, cause, interactive, command, args, nargs)
	dbref player, cause;
	int interactive;
	char *command, *args[];
	int nargs;
{
	if (!strcmp(command, "hiya")) {
		notify(player, "Got hiya.");
		return 1;
	}
	return 0;
}

int mod_hello_process_no_match(player, cause, interactive, 
				lc_command, raw_command, args, nargs)
	dbref player, cause;
	int interactive;
	char *lc_command, *raw_command, *args[];
	int nargs;
{
	if (!strcmp(lc_command, "heythere")) {
		notify(player, "Got heythere.");
		return 1;
	}
	return 0;
}

void mod_hello_create_obj(player, obj)
	dbref player, obj;
{
	notify(player, tprintf("You created #%d -- hello says so.", obj));
}

void mod_hello_destroy_obj(player, obj)
	dbref player, obj;
{
	notify(GOD, tprintf("Destroyed #%d -- hello says so.", obj));
}

void mod_hello_destroy_player(player, victim)
	dbref player, victim;
{
	notify(player, tprintf("Say goodbye to %s!", Name(victim)));
}

void mod_hello_announce_connect(player)
	dbref player;
{
	notify(GOD, tprintf("%s(#%d) just connected -- hello says so.",
				Name(player), player));
}

void mod_hello_announce_disconnect(player, reason)
	dbref player;
	const char *reason;
{
	notify(GOD, tprintf("%s(#%d) just disconnected -- hello says so.",
				Name(player), player));
}

/* --------------------------------------------------------------------------
 * Commands.
 */

DO_CMD_NO_ARG(mod_hello_do_hello)
{
    int i;

    if (key & MOD_HELLO_HELLO_INFORMAL) {
	for (i = 0; i < mod_hello_config.hello_times; i++) 
	    notify(player, "Hi there!");
    } else {
	if (mod_hello_config.show_name)
	    notify(player, tprintf("Hello, %s!", Name(player)));
	else
	    notify(player, mod_hello_config.hello_string);
    }

    mod_hello_db[player].greeted += 1;
    notify(player, tprintf("You have been greeted %d times.", 
			   mod_hello_db[player].greeted));
}

DO_CMD_NO_ARG(mod_hello_do_foof)
{
    notify(player, "Yay.");
    mod_hello_db[player].foofed += 1;
    notify(player, tprintf("You have been foofed %d times.", 
			   mod_hello_db[player].foofed));
}

NAMETAB mod_hello_hello_sw[] = {
{(char *)"informal",	1,	CA_PUBLIC,	MOD_HELLO_HELLO_INFORMAL},
{ NULL,			0,	0,		0}};

CMDENT mod_hello_cmdtable[] = {
{(char *)"@hello",		mod_hello_hello_sw,	CA_PUBLIC,
	0,		CS_NO_ARGS,
	NULL,		NULL,	NULL,			mod_hello_do_hello},
{(char *)"@foof",		NULL,			CA_PUBLIC,
	0,		CS_NO_ARGS,
	NULL,		NULL,	NULL,			mod_hello_do_foof},
{(char *)NULL,			NULL,		0,
	0,		0,
	NULL,		NULL,	NULL,		NULL}};

/* --------------------------------------------------------------------------
 * Functions.
 */

FUNCTION(mod_hello_fun_hello)
{
	safe_str("Hello, world!", buff, bufc);
}

FUN mod_hello_functable[] = {
{"HELLO",	mod_hello_fun_hello,	0,	0,	CA_PUBLIC},
{NULL,		NULL,			0,	0,	0}};

/* --------------------------------------------------------------------------
 * Hash tables.
 */

HASHTAB mod_hello_greetings;
HASHTAB mod_hello_farewells;

MODHASHES mod_hello_hashtable[] = {
{ "Hello greetings",	&mod_hello_greetings,	5,	8},
{ "Hello farewells",	&mod_hello_farewells,	15,	32},
{ NULL,			NULL,			0,	0}};

NHSHTAB mod_hello_numbers;

MODNHASHES mod_hello_nhashtable[] = {
{ "Hello numbers",	&mod_hello_numbers,	5,	16},
{ NULL,			NULL,			0,	0}};

/* --------------------------------------------------------------------------
 * Initialization.
 */
	
void mod_hello_init()
{
    mod_hello_config.show_name = 0;
    mod_hello_config.hello_string = XSTRDUP("Hello, world!", "mod_hello_init");
    mod_hello_config.hello_times = 1;

    register_hashtables(mod_hello_hashtable, mod_hello_nhashtable); 
    register_commands(mod_hello_cmdtable);
    register_functions(mod_hello_functable);
}
