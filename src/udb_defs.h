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

#define	CACHE_SIZE	2000000		/* 2 million bytes */
#define	CACHE_WIDTH	20

/* Macros for calling the DB layer */

#define	DB_GET(n)	dddb_get(n)
#define	DB_PUT(o,n)	dddb_put(o,n)
#define	DB_DEL(n)	dddb_del(n)

#endif /* __UDB_DEFS_H */
