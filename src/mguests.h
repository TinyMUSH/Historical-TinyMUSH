/* mguests.h */
/* $Id$ */

#include "copyright.h"

#ifndef __MGUESTS_H
#define __MGUESTS_H

extern char *	FDECL(make_guest, (DESC *));
extern dbref	FDECL(create_guest, (char *, char *));
extern void	FDECL(destroy_guest, (dbref));

#endif /* __MGUESTS_H */
