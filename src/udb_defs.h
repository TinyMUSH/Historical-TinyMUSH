/* udb_defs.h - Header file for the UnterMud DB layer, in TinyMUSH 3.0 */
/* $Id$ */

/*
	Andrew Molitor, amolitor@eagle.wesleyan.edu
	1991
*/

#include "copyright.h"

#ifndef __UDB_DEFS_H
#define __UDB_DEFS_H

/* If your malloc() returns void or char pointers... */
/* typedef	void	*mall_t; */
typedef	char	*mall_t;

/* default (runtime-resettable) cache parameters */

#define	CACHE_SIZE	1000000		/* 1 million bytes */
#define	CACHE_WIDTH	20

/* Macros for calling the DB layer */

#define	ATTR_GET(n)	dddb_get(n,sizeof(Aname))
#define	ATTR_PUT(o,n)	dddb_put(o,sizeof(Aname),n,strlen(n)+1)
#define	ATTR_DEL(n)	dddb_del(n,sizeof(Aname))

#endif /* __UDB_DEFS_H */
