#ifndef HAD_FILE_H
#define HAD_FILE_H

/*
  file.h -- information about one file
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include <cinttypes>
#include <string>

#include <time.h>

#include "hashes.h"
#include "types.h"


class File {
public:
    std::string name;
    std::string *filename_extension;
    std::string merge;
    uint64_t size;
    Hashes hashes;
    uint64_t size_detector;
    Hashes hashes_detector;
    time_t mtime;
    status_t status;
    where_t where;
    
    File() : filename_extension(NULL), size(SIZE_UNKNOWN), size_detector(SIZE_UNKNOWN), mtime(0), status(STATUS_OK), where(FILE_INGAME) { }
    
    uint64_t get_size(bool detector) const { return detector ? size_detector : size; }
    const Hashes &get_hashes(bool detector) const { return detector ? hashes_detector : hashes; }
    
    std::string filename() const { return filename_extension ? name + *filename_extension : name; }
    const std::string &merged_name() const { return merge.empty() ? name : merge; }
    bool is_size_known(bool detector = false) const { return get_size(detector) != SIZE_UNKNOWN; }
    
    bool compare_name(const File &other) const;
    bool compare_merged(const File &other) const;
    bool compare_name_size_hashes(const File &other) const;
    bool compare_size_hashes(const File &other) const;
    Hashes::Compare compare_hashes(const File &other) const;
    bool is_mergable(const File &other) const;
    bool size_hashes_are_set(bool detector) const;
    
    bool operator<(const File &other) const { return name < other.name; }

private:
    static std::string no_extension;
    bool compare_size_hashes_one(const File &other, bool detector) const;
};

#endif /* file.h */
