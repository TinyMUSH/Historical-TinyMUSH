/* gdbmsetopt.c - set options pertaining to a GDBM descriptor. */

/*
 * This file was modified for TinyMUSH:
 *
 * $Log$
 * Revision 1.4  2001/02/01 07:41:59  dpassmor
 * CENTFREE AND COALESCEBLKS GDBM options now properly set (thanks Robby)
 *
 * Revision 1.3  2000/02/27 06:22:03  cvs
 * Okay, it turns out gdbm really needs at least 10 buckets to write
 * efficiently, so I'm turning the clock back to the old behavior... one day
 * we'll have to fix this.
 *
 * Revision 1.2  2000/02/27 04:26:28  cvs
 * If the GDBM cachesize was set to less than 10 (it is set to 1 in the MUSH
 * source) then GDBM automatically raised it to 10. Now it will accept whatever
 * cache size it is given.
 *
 * Revision 1.1  1999/06/21 18:39:42  dpassmor
 * added gdbm-1.8.0
 */

/*  This file is part of GDBM, the GNU data base manager, by Philip A. Nelson.
    Copyright (C) 1993, 1994  Free Software Foundation, Inc.

    GDBM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    GDBM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GDBM; see the file COPYING.  If not, write to
    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

    You may contact the original author by:
       e-mail:  phil@cs.wwu.edu
      us-mail:  Philip A. Nelson
                Computer Science Department
                Western Washington University
                Bellingham, WA 98226
       
    The author of this file is:
       e-mail:  downsj@downsj.com

*************************************************************************/


/* include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "gdbmerrno.h"

/* operate on an already open descriptor. */

/* ARGSUSED */
int
gdbm_setopt(dbf, optflag, optval, optlen)
    gdbm_file_info *dbf;	/* descriptor to operate on. */
    int optflag;		/* option to set. */
    int *optval;		/* pointer to option value. */
    int optlen;			/* size of optval. */
{
  switch(optflag)
    {
      case GDBM_CACHESIZE:
        /* Optval will point to the new size of the cache. */
        if (dbf->bucket_cache != NULL)
          {
            gdbm_errno = GDBM_OPT_ALREADY_SET;
            return(-1);
          }

        return(_gdbm_init_cache(dbf, ((*optval) > 9) ? (*optval) : 10));

      case GDBM_FASTMODE:
      	/* Obsolete form of SYNCMODE. */
	if ((*optval != TRUE) && (*optval != FALSE))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return(-1);
	  }

	dbf->fast_write = *optval;
	break;

      case GDBM_SYNCMODE:
      	/* Optval will point to either true or false. */
	if ((*optval != TRUE) && (*optval != FALSE))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return(-1);
	  }

	dbf->fast_write = !(*optval);
	break;

      case GDBM_CENTFREE:
      	/* Optval will point to either true or false. */
	if ((*optval != TRUE) && (*optval != FALSE))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return(-1);
	  }

	dbf->central_free = *optval;
	break;

      case GDBM_COALESCEBLKS:
      	/* Optval will point to either true or false. */
	if ((*optval != TRUE) && (*optval != FALSE))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return(-1);
	  }

	dbf->coalesce_blocks = *optval;
	break;

      default:
        gdbm_errno = GDBM_OPT_ILLEGAL;
        return(-1);
    }

  return(0);
}
