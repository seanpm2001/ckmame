/*
  $NiH: garbage.c,v 1.3 2006/05/06 23:31:40 dillo Exp $

  garbage.c -- move files to garbage directory
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <stdlib.h>

#include "error.h"
#include "funcs.h"
#include "garbage.h"
#include "xmalloc.h"



static int garbage_open(garbage_t *);



int garbage_add(garbage_t *g, int idx)
{
    struct zip_source *source;
    const char *name;
    char *name2;

    if (g->zname == NULL)
	garbage_open(g);

    if (g->za == NULL)
	return -1;

    name = rom_name(archive_file(g->a, idx));
    if (zip_name_locate(g->za, name, 0) == 0)
	name2 = my_zip_unique_name(g->za, name);
    else
	name2 = NULL;

    if ((source=zip_source_zip(g->za, archive_zip(g->a), idx,
			       ZIP_FL_UNCHANGED, 0, -1)) == NULL
	|| zip_add(g->za, name2 ? name2 : name, source) < 0) {
	zip_source_free(source);
	seterrinfo(archive_name(g->a), rom_name(archive_file(g->a, idx)));
	myerror(ERRZIPFILE, "error moving to `%s': %s",
		g->zname, zip_strerror(g->za));
	free(name2);
	return -1;
    }

    free(name2);
    return 0;
}



int garbage_close(garbage_t *g)
{
    struct zip *za;
    char *zname;

    if (g == NULL)
	return 0;

    za = g->za;
    zname = g->zname;

    free(g);

    if (za == NULL) {
	free(zname);
	return 0;
    }

    if (zip_get_num_files(za) > 0) {
	if (ensure_dir(zname, 1) < 0) {
	    zip_unchange_all(za);
	    zip_close(za);
	    free(zname);
	    return -1;
	}
    }

    free(zname);

    if (zip_close(za) < 0) {
	zip_unchange_all(za);
	zip_close(za);
	return -1;
    }

    return 0;
}



garbage_t *garbage_new(archive_t *a)
{
    garbage_t *g;

    g = (garbage_t *)xmalloc(sizeof(*g));

    g->a = a;
    g->zname = NULL;
    g->za = NULL;

    return g;
}



static int
garbage_open(garbage_t *g)
{
    g->zname = make_garbage_name(archive_name(g->a), 0);

    if ((g->za=my_zip_open(g->zname, ZIP_CREATE)) == NULL)
	return -1;

    return 0;
}
