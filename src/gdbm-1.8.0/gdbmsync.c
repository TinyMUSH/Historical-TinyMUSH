/* gdbmsync.c - Sync the disk with the in memory state. */

/*  This file is part of GDBM, the GNU data base manager, by Philip A. Nelson.
    Copyright (C) 1990, 1991, 1993  Free Software Foundation, Inc.

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

    You may contact the author by:
       e-mail:  phil@cs.wwu.edu
      us-mail:  Philip A. Nelson
                Computer Science Department
                Western Washington University
                Bellingham, WA 98226
       
*************************************************************************/


/* include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "gdbmerrno.h"

#define TM3

#ifdef TM3
extern char *data, *dptr;
extern int size;
extern off_t start_file_adr;
#endif

/* Make sure the database is all on disk. */

void
gdbm_sync (dbf)
     gdbm_file_info *dbf;
{
#ifdef TM3
  off_t file_pos;
  int num_bytes;
#endif

  /* Initialize the gdbm_errno variable. */
  gdbm_errno = GDBM_NO_ERROR;
#ifdef TM3
  if (size) {
    file_pos = lseek (dbf->desc, start_file_adr, L_SET);
    if (file_pos != start_file_adr) _gdbm_fatal (dbf, "lseek error");
    num_bytes = write (dbf->desc, data, size);
    if (num_bytes != size) _gdbm_fatal (dbf, "write error");
    start_file_adr = 0;
    size = 0;
    dptr = data;
  }
#endif

  /* Do the sync on the file. */
  fsync (dbf->desc);

}
