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

void hello_do_cmd(player, cause, key)
dbref player, cause;
int key;
{
	notify(player, "Hello world-- from TinyMUSH 3!");
}

CMDENT hello_cmd =
{(char *)"@hello",		NULL,		CA_PUBLIC,
	0,		CS_NO_ARGS,
	NULL,		NULL,	NULL,		hello_do_cmd};	

void init_hello()
{
	hashadd("@hello", (int *) &hello_cmd, &mudstate.command_htab);
}

