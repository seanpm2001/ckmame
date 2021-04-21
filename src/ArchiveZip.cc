/*
 ArchiveZip.cc -- implementation of archive from zip
 Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include "ArchiveZip.h"

#include <sys/stat.h>
#include <cerrno>

#include "Detector.h"
#include "error.h"
#include "Exception.h"
#include "util.h"
#include "zip_util.h"


#define BUFSIZE 8192

bool ArchiveZip::ensure_zip() {
    if (za != NULL) {
        return true;
    }

    int zip_flags = (contents->flags & ARCHIVE_FL_CHECK_INTEGRITY) ? ZIP_CHECKCONS : 0;
    if (contents->flags & ARCHIVE_FL_CREATE)
	zip_flags |= ZIP_CREATE;

    int err;
    if ((za = zip_open(name.c_str(), zip_flags, &err)) == NULL) {
        char errbuf[80];

        zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
        myerror(ERRDEF, "error %s zip archive '%s': %s", (contents->flags & ZIP_CREATE ? "creating" : "opening"), name.c_str(), errbuf);
	return false;
    }
    
    return true;
}


bool ArchiveZip::check() {
    return ensure_zip();
}


bool ArchiveZip::close_xxx() {
    if (za == NULL) {
        return true;
    }

    if (zip_close(za) < 0) {
	/* error closing, so zip is still valid */
	myerror(ERRZIP, "error closing zip: %s", zip_strerror(za));

	/* TODO: really do this here? */
	/* discard all changes and close zipfile */
	zip_discard(za);
        za = NULL;
        return false;
    }

    za = NULL;

    return true;
}


bool ArchiveZip::commit_xxx() {
    if (((contents->flags & ARCHIVE_FL_RDONLY) == 0) && za != NULL && !files.empty()) {
        if (!ensure_dir(name, true)) {
            return false;
        }
    }
    
    if (!ensure_zip()) {
        return false;
    }
    
    auto ok = true;
    
    for (size_t index = 0; index < files.size(); index++) {
        auto &file = files[index];
        auto &change = contents->changes[index];
        
        if (file.where == FILE_DELETED) {
            if (zip_delete(za, index) < 0) {
                myerror(ERRZIP, "cannot delete '%s': %s", file.name.c_str(), zip_strerror(za));
                ok = false;
                break;
            }
        }
        else if (file.where == FILE_ADDED) {
            if (!ensure_file_doesnt_exist(file.name)) {
                ok = false;
                break;
            }
            zip_source_keep(change.source->source);
            if (zip_file_add(za, file.name.c_str(), change.source->source, 0) < 0) {
                zip_source_free(change.source->source);
                if (change.source_name.empty()) {
                    myerror(ERRZIPFILE, "error adding empty file: %s", zip_strerror(za));
                }
                else {
                    myerror(ERRZIPFILE, "error adding '%s': %s", change.source_name.c_str(), zip_strerror(za));
                }
                ok = false;
                break;
            }
        }
        else {
            if (!change.original_name.empty()) {
                if (!ensure_file_doesnt_exist(file.name)) {
                    ok = false;
                    break;
                }
                if (zip_rename(za, index, file.name.c_str()) < 0) {
                    myerror(ERRZIP, "cannot rename '%s' to `%s': %s", change.original_name.c_str(), file.name.c_str(), zip_strerror(za));
                    ok = false;
                    break;
                }
            }
            if (change.source) {
                zip_source_keep(change.source->source);
                if (zip_file_replace(za, index, change.source->source, 0) < 0) {
                    zip_source_free(change.source->source);
                    if (change.source_name.empty()) {
                        myerror(ERRZIPFILE, "error adding empty file: %s", zip_strerror(za));
                    }
                    else {
                        myerror(ERRZIPFILE, "error adding '%s': %s", change.source_name.c_str(), zip_strerror(za));
                    }
                    ok = false;
                    break;
                }
            }
        }
    }

    if (!ok) {
        zip_unchange_all(za);
        return false;
    }
    
    return close_xxx();
}


void ArchiveZip::commit_cleanup() {
    if (files.empty()) {
	return;
    }

    ensure_zip();

    for (uint64_t i = 0; i < files.size(); i++) {
	struct zip_stat st;

	if (zip_stat_index(za, i, 0, &st) < 0) {
	    seterrinfo("", name);
	    myerror(ERRZIP, "cannot stat file %" PRIu64 ": %s", i, zip_strerror(za));
	    continue;
	}

        files[i].mtime = st.mtime;
    }
}


void ArchiveZip::get_last_update() {
    struct stat st;
    if (stat(name.c_str(), &st) < 0) {
        contents->size = 0;
        contents->mtime = 0;
        return;
    }

    contents->mtime = st.st_mtime;
    contents->size = static_cast<uint64_t>(st.st_size);
}


bool ArchiveZip::read_infos_xxx() {
    struct zip_stat zsb;

    if (!ensure_zip()) {
        return false;
    }
    
    seterrinfo("", name);

    zip_uint64_t n = static_cast<zip_uint64_t>(zip_get_num_entries(za, 0));
    
    for (zip_uint64_t i = 0; i < n; i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %" PRIu64 ": %s", i, zip_strerror(za));
	    continue;
	}

        File r;
        r.mtime = zsb.mtime;
	r.size = zsb.size;
	r.name = zsb.name;
        r.status = STATUS_OK;
        r.hashes.set_crc(zsb.crc);
        
        files.push_back(r);

        if (detector) {
	    file_match_detector(i);
        }

        if (contents->flags & ARCHIVE_FL_CHECK_INTEGRITY) {
	    file_ensure_hashes(i, contents->flags & ARCHIVE_FL_HASHTYPES_MASK);
        }
    }
    
    return true;
}
                                        

ZipSourcePtr ArchiveZip::get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length_) {
    if (!ensure_zip()) {
        return NULL;
    }
    
    // TODO: overflow check
    int64_t length = static_cast<int64_t>(length_.has_value() ? length_.value() : files[index].size - start);
    
    auto source = zip_source_zip_create(za, index, ZIP_FL_UNCHANGED, start, length, NULL);
    
    if (source == NULL) {
        // TODO: error message
        return {};
    }

    return std::make_shared<ZipSource>(source);
}


bool ArchiveZip::ensure_file_doesnt_exist(const std::string &filename) {
    auto index = zip_name_locate(za, filename.c_str(), 0);

    if (index >= 0) {
        auto new_name = make_unique_name_in_archive(filename);
        if (zip_rename(za, static_cast<uint64_t>(index), new_name.c_str()) < 0) {
            seterrinfo(filename, name);
            myerror(ERRFILE, "can't move out of the way: %s", zip_error_strerror(zip_get_error(za)));
            return false;
        }
    }
    
    return true;
}
