/*
	Header file for the UnterMud DB layer, as applied to TinyMUSH 3.0

	Andrew Molitor, amolitor@eagle.wesleyan.edu
	1991
	
	$Id$
*/

#ifdef VMS
#define MAXPATHLEN	256
#define	NOSYSTYPES_H
#define	NOSYSFILE_H
#endif /* VMS */

/* If your malloc() returns void or char pointers... */
/* typedef	void	*mall_t; */
typedef	char	*mall_t;

/* default (runtime-resettable) cache parameters */

#define CACHE_DEPTH	10
#define	CACHE_WIDTH	20

/* Macros for calling the DB layer */

#define	DB_GET(n)	dddb_get(n)
#define	DB_PUT(o,n)	dddb_put(o,n)
#define	DB_DEL(n,f)	dddb_del(n)
