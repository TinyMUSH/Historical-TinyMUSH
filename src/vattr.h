/* vattr.h - Definitions for user-defined attributes */
/* $Id$ */

#include "copyright.h"

#ifndef __VATTR_H
#define __VATTR_H

#ifndef VATTR_HASH_SIZE		/* must be a power of two */
#define VATTR_HASH_SIZE 16384
#endif

#define VNAME_SIZE	32

typedef struct user_attribute VATTR;
struct user_attribute {
	char	*name; /* Name of user attribute */
	int	number;		/* Assigned attribute number */
	int	flags;		/* Attribute flags */
};

extern void	NDECL(vattr_init);
extern VATTR *	FDECL(vattr_rename, (char *, char *));
extern VATTR *	FDECL(vattr_find, (char *));
extern VATTR *	FDECL(vattr_nfind, (int));
extern VATTR *	FDECL(vattr_alloc, (char *, int));
extern VATTR *	FDECL(vattr_define, (char *, int, int));
extern void	FDECL(vattr_delete, (char *));
extern VATTR *	FDECL(attr_rename, (char *, char *));
extern VATTR *	NDECL(vattr_first);
extern VATTR *	FDECL(vattr_next, (VATTR *));
extern void	FDECL(list_vhashstats, (dbref));

#endif /* __VATTR_H */
