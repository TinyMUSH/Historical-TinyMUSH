/* mguests.h */
/* $Id$ */

#ifndef  __MGUESTS_H
#define __MGUESTS_H

#include "copyright.h"
#include "autoconf.h"
#include "interface.h"

extern char *	FDECL(make_guest, (DESC *));
extern dbref	FDECL(create_guest, (char *, char *));
extern void	FDECL(destroy_guest, (dbref));
#endif
