/* hello.c - demonstration module */
/* $Id$ */

#include "../api.h"

#define MOD_HELLO_HELLO_INFORMAL	1

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

void mod_hello_init()
{
    register_commands(mod_hello_cmdtable);
}
