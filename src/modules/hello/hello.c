/* hello.c - demonstration module */
/* $Id$ */

#include "../api.h"

#define MOD_HELLO_HELLO_INFORMAL	1

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
    if (key & MOD_HELLO_HELLO_INFORMAL) {
	notify(player, "Hi there!");
    } else {
	notify(player, "Hello world!");
    }
}

DO_CMD_NO_ARG(mod_hello_do_foof)
{
    notify(player, "Yay.");
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
 * Initialization.
 */
	
void mod_hello_init()
{
    register_commands(mod_hello_cmdtable);
    register_functions(mod_hello_functable);
}
