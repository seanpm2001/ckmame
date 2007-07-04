#ifndef HAD_UINT32_H
#define HAD_UINT32_H

/*
  $NiH: uint32.h,v 1.2 2005/12/27 11:11:26 wiz Exp $

  uint32.h -- ensure that uint32_t is defined to a 4-byte type
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include "config.h"

#ifndef HAVE_UINT32_T
# if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
# elif defined(HAVE_STDINT_H)
#include <stdint.h>
# else
#  if SIZEOF_UNSIGNED_INT == 4
typedef unsigned int uint32_t
#  elif SIZEOF_UNSIGNED_LONG == 4
typedef unsigned long uint32_t
#  else
#error no 4-byte unsigned integer type found
#  endif
# endif /* !HAVE_STDINT_H && !HAVE_INTTYPES_H */
#endif /* HAVE_UINT32_t */

#endif /* uint32.h */
