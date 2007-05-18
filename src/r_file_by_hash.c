/*
  $NiH: r_file_by_hash.c,v 1.5 2006/04/15 22:52:58 dillo Exp $

  r_file_location.c -- read file_by_hash information from db
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "dbh.h"
#include "file_location.h"
#include "xmalloc.h"

array_t *
r_file_by_hash(sqlite3 *db, filetype_t ft, const hashes_t *hash)
{
#if 0
    DBT v;
    array_t *a;
    void *data;

    if (dbh_lookup(db, file_location_make_key(ft, hash), &v) != 0)
	return NULL;

    data = v.data;

    a = r__array(&v, r__file_location, sizeof(file_location_t));
    
    free(data);

    return a;
#else
    return NULL;
#endif
}
