#include "../autoconf.h"
#include "../flags.h"
#include "../alloc.h"
#include "../config.h"
#include "../htab.h"
#include "../command.h"
#include "../mail.h"
#include "../mudconf.h"
#include "../db.h"
#include "../externs.h"

void mod_hello_do_cmd(player, cause, key)
dbref player, cause;
int key;
{
	notify(player, "Hello world-- from TinyMUSH 3!");
}

CMDENT mod_hello_cmd =
{(char *)"@hello",		NULL,		CA_PUBLIC,
	0,		CS_NO_ARGS,
	NULL,		NULL,	NULL,		mod_hello_do_cmd};	

void mod_hello_init()
{
	hashadd("@hello", (int *) &mod_hello_cmd, &mudstate.command_htab);
}

