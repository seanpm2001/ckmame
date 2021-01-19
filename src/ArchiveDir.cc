/*
 archive_dir.c -- implementation of archive from directory
 Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include <filesystem>

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "funcs.h"

#include "ArchiveDir.h"
#include "Dir.h"
#include "error.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"

bool ArchiveDir::Change::is_renamed() const {
    if (original.data_file_name.empty() || destination.data_file_name.empty()) {
        return false;
    }
    return original.data_file_name == destination.data_file_name;
}

bool ArchiveDir::Change::has_new_data() const {
    if (destination.name.empty()) {
        return false;
    }

    return (original.name.empty() || original.data_file_name != destination.data_file_name);
}

bool ArchiveDir::FileInfo::apply() const {
    if (name.empty() || name == data_file_name) {
        return true;
    }

    if (rename(data_file_name.c_str(), name.c_str()) < 0) {
        myerror(ERRZIP, "apply: cannot rename '%s' to '%s': %s", data_file_name.c_str(), name.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool ArchiveDir::FileInfo::discard(ArchiveDir *archive) const {
    std::error_code ec;

    if (name.empty() || name == data_file_name) {
        return true;
    }

    std::filesystem::remove(data_file_name, ec);
    if (ec) {
        myerror(ERRZIP, "cannot delete '%s': %s", data_file_name.c_str(), ec.message().c_str());
	return false;
    }
    auto path = std::filesystem::path(name).parent_path();
    /* TODO: stop before removing archive_name itself, e.g. extra_dir */
    while (!path.empty()) {
	std::filesystem::remove(path, ec);
	if (ec) {
	    break;
	}
	path = path.parent_path();
    }

    return true;
}


/* returns: -1 on error, 1 if file was moved, 0 if nothing needed to be done */
int ArchiveDir::move_original_file_out_of_the_way(uint64_t index) {
    auto change = get_change(index, true);
    auto filename = files[index].name;

    if (change->is_added() || !change->original.name.empty()) {
        return 0;
    }

    auto full_name = get_full_name(index);
    auto tmp = make_tmp_name(filename);

    if (rename(full_name.c_str(), tmp.c_str()) < 0) {
        myerror(ERRZIP, "move: cannot rename '%s' to '%s': %s", filename.c_str(), tmp.c_str(), strerror(errno));
        return -1;
    }

    change->original.name = full_name;
    change->original.data_file_name = tmp;

    return 1;
}


bool ArchiveDir::Change::apply(ArchiveDir *archive, uint64_t index) {
    if (!destination.name.empty()) {
        if (!ensure_dir(destination.name, true)) {
            myerror(ERRZIP, "destination directory cannot be created: %s", strerror(errno));
            return false;
        }

        if (!destination.apply()) {
            return false;
        }

        struct stat st;
        if (stat(destination.name.c_str(), &st) < 0) {
            myerror(ERRZIP, "can't stat created file '%s': %s", destination.name.c_str(), strerror(errno));
            return false;
        }
        archive->files[index].mtime = st.st_mtime;
    }

    if (!is_renamed()) {
        if (!original.discard(archive)) {
            return false;
        }
    }

    clear();

    return true;
}

void ArchiveDir::Change::rollback(ArchiveDir *archive) {
    original.apply();

    if (!is_renamed()) {
        destination.discard(archive);
    }

    clear();
}

void ArchiveDir::Change::clear() {
    original.clear();
    destination.clear();
    mtime = 0;
}


void ArchiveDir::FileInfo::clear() {
    name.clear();
    data_file_name.clear();
}


bool ArchiveDir::ensure_archive_dir() {
    return ensure_dir(name, false);
}


ArchiveDir::Change *ArchiveDir::get_change(uint64_t index, bool create) {
    if (changes.size() <= index) {
        if (!create) {
            return NULL;
        }
        else {
            changes.resize(index + 1);
        }
    }
    
    return &changes[index];
}

std::filesystem::path ArchiveDir::get_full_name(uint64_t index) {
    auto change = get_change(index);
    
    if (change != NULL && !change->destination.data_file_name.empty()) {
        return change->destination.data_file_name;
    }

    return make_full_name(files[index].name);
}

std::filesystem::path ArchiveDir::get_original_data(uint64_t index) {
    auto change = get_change(index);
    
    if (change != NULL && !change->original.data_file_name.empty()) {
        return change->original.data_file_name;
    }

    return make_full_name(files[index].name);
}


std::filesystem::path ArchiveDir::make_full_name(const std::filesystem::path &filename) {
    return std::filesystem::path(name) /= filename;
}

std::filesystem::path ArchiveDir::make_tmp_name(const std::filesystem::path &filename) {
    auto name_template = static_cast<std::string>(std::filesystem::path(name) / (static_cast<std::string>(filename) + ".XXXXX"));

    char *c_string = xstrdup(name_template.c_str());
    
    for (size_t i = name.length() + 1; i < name_template.length(); i++) {
        if (c_string[i] == '/') {
            c_string[i] = '_';
        }
    }
    
    if (mktemp(c_string) == NULL) {
        free(c_string);
        return "";
    }

    auto tmp_name = std::filesystem::path(c_string);
    free(c_string);
    return tmp_name;
}


bool ArchiveDir::commit_xxx() {
    if (!modified) {
        return true;
    }

    bool is_empty = true;

    for (auto &file : files) {
        if (file.where != FILE_DELETED) {
            is_empty = false;
            break;
        }
    }

    auto ok = true;
    
    for (size_t index = 0; index < changes.size(); index++) {
        if (!changes[index].apply(this, index)) {
            ok = false;
        }
    }
    
    if (is_empty && is_writable() && !(contents->flags & (ARCHIVE_FL_KEEP_EMPTY | ARCHIVE_FL_TOP_LEVEL_ONLY))) {
	std::error_code ec;
	std::filesystem::remove(name, ec);
	if (ec) {
            myerror(ERRZIP, "cannot remove empty archive '%s': %s", name.c_str(), ec.message().c_str());
            ok = false;
        }
    }

#if 0
    if (archive_is_modified(a))
    ud->dbh_changed = true;
#endif

    return ok;
}


void ArchiveDir::commit_cleanup() {
    changes.resize(files.size());
}


bool ArchiveDir::file_add_empty_xxx(const std::string &filename) {
    return file_copy_xxx({}, NULL, 0, filename, 0, 0);
}


bool ArchiveDir::file_copy_xxx(std::optional<uint64_t> index, Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length) {
    if (!ensure_archive_dir()) {
        return false;
    }
    auto tmpname = make_tmp_name(filename);
    if (tmpname.empty()) {
        return false;
    }
    
    /* archive layer already grew archive files if didx < 0 */
    auto real_index = index.has_value() ? index.value() : files.size() - 1;
    
    File *f = &files[real_index];

    if (source_archive != NULL) {
        auto sa = static_cast<ArchiveDir *>(source_archive);
        auto source_name = sa->get_original_data(source_index);
        if (source_name.empty()) {
            return false;
        }

        if (start == 0 && (!length.has_value() || length == sa->files[source_index].size)) {
            if (link_or_copy(source_name.c_str(), tmpname.c_str()) < 0) {
                return false;
            }
        }
        else {
            if (copy_file(source_name.c_str(), tmpname.c_str(), start, static_cast<ssize_t>(length.value()), &f->hashes) < 0) {
                myerror(ERRZIP, "cannot copy '%s' to '%s': %s", source_name.c_str(), tmpname.c_str(), strerror(errno));
                return false;
            }
        }
    }
    else {
        FILE *fout;
        
        if ((fout = fopen(tmpname.c_str(), "w")) == NULL) {
            myerror(ERRZIP, "cannot open '%s': %s", tmpname.c_str(), strerror(errno));
            return false;
        }
        fclose(fout);
    }

    auto err = false;
    
    Change *change = get_change(real_index, true);
    
    if (index.has_value()) {
        if (!change->is_added()) {
            if (filename != files[real_index].name) {
                if (move_original_file_out_of_the_way(real_index) < 0) {
                    err = true;
                }
            }
            else {
                if (change->is_unchanged()) {
                    change->original.name = make_full_name(filename);
                    change->original.data_file_name = change->original.name;
                }
            }
        }
    }
    
    if (err) {
        unlink(tmpname.c_str());
        return false;
    }
    
    auto full_name = make_full_name(filename);
    
    if (change->has_new_data()) {
        change->destination.discard(this);
    }
    change->destination.name = full_name;
    change->destination.data_file_name = tmpname;
    
    return true;
}


bool ArchiveDir::file_delete_xxx(uint64_t index) {
    auto change = get_change(index, true);

    if (move_original_file_out_of_the_way(index) == -1) {
        return false;
    }

    auto ok = true;

    if (change->has_new_data()) {
        if (!change->destination.discard(this)) {
            ok = false;
        }
    }
    change->destination.clear();

    return ok;
}


Archive::ArchiveFilePtr ArchiveDir::file_open(uint64_t index) {

    FILE *f;

    if ((f = fopen(get_full_name(index).c_str(), "rb")) == NULL) {
        seterrinfo("", name);
        myerror(ERRZIP, "cannot open '%s': %s", files[index].name.c_str(), strerror(errno));
        return NULL;
    }

    return Archive::ArchiveFilePtr(new ArchiveFile(f));
}

void ArchiveDir::ArchiveFile::close() {
    if (f != NULL) {
        fclose(f);
        f = NULL;
    }
}


int64_t ArchiveDir::ArchiveFile::read(void *data, uint64_t length) {
    if (length > SIZE_T_MAX) {
        errno = EINVAL;
        return -1;
    }
    return static_cast<int64_t>(fread(data, 1, length, f));
}


bool ArchiveDir::file_will_exist_after_commit(std::filesystem::path filename) {
    for (auto &change : changes) {
        if (change.destination.name == filename) {
            return true;
        }
    }

    if (std::filesystem::exists(filename)) {
        return true;
    }

    return false;
}


bool ArchiveDir::file_rename_xxx(uint64_t index, const std::string &filename) {
    auto change = get_change(index, true);

    if (change->is_deleted()) {
        myerror(ERRZIP, "cannot rename deleted file '%s'", files[index].name.c_str());
        return false;
    }

    auto final_name = make_full_name(filename);

    if (file_will_exist_after_commit(final_name)) {
        errno = EEXIST;
        myerror(ERRZIP, "cannot rename '%s' to '%s': %s", files[index].name.c_str(), filename.c_str(), strerror(errno));
        return false;
    }

    switch (move_original_file_out_of_the_way(index)) {
        case -1:
            return false;

        case 1:
            change->destination.data_file_name = change->original.data_file_name;
            break;

        default:
            break;
    }

    change->destination.name = final_name;

    return true;
}


const char *ArchiveDir::ArchiveFile::strerror() {
    return ::strerror(errno);
}


bool ArchiveDir::read_infos_xxx() {
    try {
	 Dir dir(name, contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? false : true);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     if (name == filepath) {
		 continue;
	     }
	     if (std::filesystem::is_regular_file(filepath)) {
		 if (filepath.filename() == DBH_CACHE_DB_NAME) {
		     continue;
		 }
		 struct stat sb;

		 if (stat(filepath.c_str(), &sb) != 0) {
		     continue;
		 }

		 files.push_back(File());
		 auto &f = files[files.size() - 1];

		 f.name = filepath.string().substr(name.size() + 1);
		 // f.size = static_cast<uint64_t>(sb.st_size);
		 f.size = std::filesystem::file_size(filepath);
		 // auto ftime = std::filesystem::last_write_time(filepath);
		 // f.mtime = decltype(ftime)::clock::to_time_t(ftime);
		 f.mtime = sb.st_mtime;
	     }
	 }
    }
    catch (...) {
	return false;
    }

    return true;
}


bool ArchiveDir::rollback_xxx() {
    for (auto &change : changes) {
        change.rollback(this);
    }

    return true;
}


void ArchiveDir::get_last_update() {
    struct stat st;

    contents->size = 0;
    if (stat(name.c_str(), &st) < 0) {
        contents->mtime = 0;
        return;
    }

    contents->mtime = st.st_mtime;
}
