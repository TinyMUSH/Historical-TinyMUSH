/* udb_obj.c - Binary object handling gear. Shit simple. */
/* $Id$ */

/* 
 * Andrew Molitor, amolitor@nmsu.edu
 * 
 * 1992
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mail.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "udb.h"	/* required by code */
#include "udb_defs.h"	/* required by code */

#ifndef STANDALONE
extern void	FDECL(dump_database_internal, (int));
#endif

/* Sizes, on disk, of Object and (within the object) Attribute headers */

#define OBJ_HEADER_SIZE		(sizeof(Objname) + (2 * sizeof(int)))
#define ATTR_HEADER_SIZE	(sizeof(int) * 2)
/*
 * Routines to get Obj's on and off disk. Obj's are stowed in a
 * fairly complex way: an object header with the object ID (not really
 * needed, but there to facilitate possible recoveries of crashed DBs),
 * and an attribute count. This is followed by the attributes.
 * 
 * We use the standard library here, and do a lot of fread()s.
 * Trust your standard library. If you think this is inefficient, you have
 * your head up your ass. This means you, Jellan.
 */


Obj *
 objfromFILE(buff)
char *buff;
{
	int i, j, k;
	Obj *o;
	Attrib *a;
	char *bp;
	
	/* Get a new Obj struct */

	if ((o = (Obj *) XMALLOC(sizeof(Obj), "objfromFILE.o")) == (Obj *) 0)
		return ((Obj *) 0);

	bp = buff;
	
	/* Read in the header */

	bcopy(bp, (char *) &(o->name), sizeof(Objname));
	bp += sizeof(Objname);
	bcopy(bp, (char *) &i, sizeof(int));
	bp += sizeof(int);
	o->at_count = i;
	bcopy(bp, (char *) &k, sizeof(int));
	bp += sizeof(int);
	o->size = k;

	/* Now get an array of Attrs */

	a = o->atrs = (Attrib *) XMALLOC(i * sizeof(Attrib), "objfromFILE.a");
	if (!o->atrs) {
		XFREE(o, "objfromFILE.o");
		return ((Obj *) 0);
	}
	/* Now go get the attrs, one at a time. */

	for (j = 0; j < i;) {

		/* Attribute size */

		bcopy(bp, (char *) &(a[j].size), sizeof(int));
		bp += sizeof(int);

		/* Attribute number */

		bcopy(bp, (char *) &(a[j].attrnum), sizeof(int));
		bp += sizeof(int);

		/* get some memory for the data */

		if ((a[j].data = (char *)XMALLOC(a[j].size, "objfromFILE.d")) == (char *)0)
			goto bail;

		/* Preincrement j, so we know how many to free if this next
		 * bit fails. 
		 */

		j++;

		/* Now get the data */

		bcopy(bp, (char *) a[j - 1].data, a[j - 1].size);
		bp += a[j - 1].size;

	}


	/* Should be all done.. */

	return (o);

	/* Oh shit. We gotta free up all these little bits of memory. */
      bail:
	/* j points one attribute *beyond* what we need to free up */

	for (i = 0; i < j; i++)
		XFREE(a[i].data, "objfromFILE.d");

	XFREE(a, "objfromFILE.a");
	XFREE(o, "objfromFILE.o");

	return ((Obj *) 0);
}


int
objtoFILE(o, buff)
Obj *o;
char *buff;
{
	int i;
	Attrib *a;
	char *bp;
	
	bp = buff;

	/* Write out the object header */

	bcopy((char *) &(o->name), bp, sizeof(Objname));
	bp += sizeof(Objname);
	
	bcopy((char *)&(o->at_count), bp, sizeof(int));
	bp += sizeof(int);

	bcopy((char *)&(o->size), bp, sizeof(int));
	bp += sizeof(int);

	/* Now do the attributes, one at a time. */

	a = o->atrs;
	for (i = 0; i < o->at_count; i++) {

		/* Attribute size. */

		bcopy((char *) &(a[i].size), bp, sizeof(int));
		bp += sizeof(int);
		
		/* Attribute number */

		bcopy((char *) &(a[i].attrnum), bp, sizeof(int));
		bp += sizeof(int);

		/* Attribute data */

		bcopy((char *) a[i].data, bp, a[i].size);
		bp += a[i].size;
	}

	return (0);
}

/* Return the size, on disk, the thing is going to take up. */

int
obj_siz(o)
Obj *o;
{
	int i;
	int siz;

	siz = OBJ_HEADER_SIZE;

	for (i = 0; i < o->at_count; i++)
		siz += (((o->atrs)[i]).size + ATTR_HEADER_SIZE);

	return (siz);
}
