/*
  ckmame.c -- main routine for ckmame
  Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner

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

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

#include "compat.h"
#include "config.h"

#include "check_util.h"
#include "cleanup.h"
#include "CkmameDB.h"
#include "Commandline.h"
#include "Configuration.h"
#include "diagnostics.h"
#include "error.h"
#include "Exception.h"
#include "fix.h"
#include "fixdat.h"
#include "globals.h"
#include "MemDB.h"
#include "RomDB.h"
#include "sighandle.h"
#include "Stats.h"
#include "superfluous.h"
#include "Tree.h"
#include "util.h"
#include "warn.h"


/* to identify roms directory uniquely */
std::string rom_dir_normalized;

const char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner";
const char help_footer[] = "Report bugs to " PACKAGE_BUGREPORT ".";
const char version_string[] = PACKAGE " " VERSION "\n"
				"Copyright (C) 1999-2022 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

std::vector<Commandline::Option> options = {
    Commandline::Option("help", 'h', "display this help message"),
    Commandline::Option("version", 'V', "display version number"),

    Commandline::Option("autofixdat", "write fixdat to 'fix_$NAME_OF_SET.dat'"),
    Commandline::Option("complete-games-only", 'C', "only keep complete games in ROM set"),
    Commandline::Option("config", "file", "read configuration from file"),
    Commandline::Option("copy-from-extra", "keep used files in extra directories (default)"),
    Commandline::Option("db", 'D', "dbfile", "use mame-db dbfile"),
    Commandline::Option("extra-directory", 'e', "dir", "search for missing files in directory dir (multiple directories can be specified by repeating this option)"),
    Commandline::Option("fix", 'F', "fix ROM set"),
    Commandline::Option("fixdat", "datfile", "write fixdat to 'datfile'"),
    Commandline::Option("game-list", 'T', "file", "read games to check from file"),
    Commandline::Option("keep-old-duplicate", "keep files in ROM set that are also in old ROMs"),
    Commandline::Option("move-from-extra", 'j', "remove used files from extra directories"),
    Commandline::Option("no-complete-games-only", "keep partial games in ROM set (default)"),
    Commandline::Option("no-report-correct", "don't report status of ROMs that are correct (default)"),
    Commandline::Option("no-report-detailed", "don't report status of every ROM (default)"),
    Commandline::Option("no-report-fixable", "don't report status of ROMs that can be fixed"),
    Commandline::Option("no-report-missing", "don't report status of ROMs that are missing"),
    Commandline::Option("no-report-summary", "don't print summary of ROM set status (default)"),
    Commandline::Option("old-db", 'O', "dbfile", "use mame-db dbfile for old ROMs"),
    Commandline::Option("report-correct", 'c', "report status of ROMs that are correct"),
    Commandline::Option("report-detailed", "report status of every ROM"),
    Commandline::Option("report-fixable", "report status of ROMs that can be fixed (default)"),
    Commandline::Option("report-missing", "report status of ROMs that are missing (default)"),
    Commandline::Option("report-summary", "print summary of ROM set status"),
    Commandline::Option("rom-dir", 'R', "dir", "ROM set is in directory dir (default: 'roms')"),
    Commandline::Option("roms-unzipped", 'u', "ROMs are files on disk, not contained in zip archives"),
    Commandline::Option("set", "name", "check ROM set name"),
    Commandline::Option("verbose", 'v', "print fixes made")
};



static bool contains_romdir(const std::string &ame);

int
main(int argc, char **argv) {
    setprogname(argv[0]);

    try {
	int found;
	std::string game_list;
	auto auto_fixdat = false;
	std::vector<std::string> arguments;
		
	auto commandline = Commandline(options, "[game ...]", help_head, help_footer);

	try {
	    auto args = commandline.parse(argc, argv);
	
	    if (args.find_first("help").has_value()) {
		commandline.usage(true);
		exit(0);
	    }
	    if (args.find_first("version").has_value()) {
		fputs(version_string, stdout);
		exit(0);
	    }

	    std::string set;
	    auto set_exists = true;
	    
	    for (const auto &option : args.options) {
		if (option.name == "set") {
		    set = option.argument;
		    set_exists = false;
		}
	    }

	    if (configuration.merge_config_file(Configuration::user_config_file(), set, true)) {
		set_exists = true;
	    }
	    if (configuration.merge_config_file(Configuration::local_config_file(), set, true)) {
		set_exists = true;
	    }
	    
	    for (const auto &option : args.options) {
		if (option.name == "config") {
		    if (configuration.merge_config_file(option.argument, set, false)) {
			set_exists = true;
		    }
		}
	    }
	    
	    if (!set_exists) {
		myerror(ERRDEF, "unknown set '%s'", set.c_str());
		exit(1);
	    }

	    auto extra_directory_specified = false;
	    
	    for (const auto &option : args.options) {
		if (option.name == "autofixdat") {
		    auto_fixdat = true;
		}
		else if (option.name == "complete-games-only") {
		    configuration.complete_games_only = true;
		}
		else if (option.name == "copy-from-extra") {
		    configuration.move_from_extra = false;
		}
		else if (option.name == "db") {
		    configuration.romdb_name = option.argument;
		}
		else if (option.name == "extra-directory") {
		    if (!extra_directory_specified) {
			configuration.extra_directories.clear();
			extra_directory_specified = true;
		    }
		    std::string name = option.argument;
		    auto last = name.find_last_not_of('/');
		    if (last == std::string::npos) {
			name = "/";
		    }
		    else {
			name.resize(last + 1);
		    }
		    configuration.extra_directories.push_back(name);
		}
		else if (option.name == "fix") {
		    configuration.fix_romset = true;
		}
		else if (option.name == "fixdat") {
		    configuration.fixdat = option.argument;
		}
		else if (option.name == "game-list") {
		    game_list = option.argument;
		}
		else if (option.name == "keep-old-duplicate") {
		    configuration.keep_old_duplicate = true;
		}
		else if (option.name == "move-from-extra") {
		    configuration.move_from_extra = true;
		}
		else if (option.name == "old-db") {
		    configuration.olddb_name = option.argument;
		}
		else if (option.name == "no-complete-games-only") {
		    configuration.complete_games_only = false;
		}
		else if (option.name == "no-report-correct") {
		    configuration.report_correct = false;
		}
		else if (option.name == "no-report-detailed") {
		    configuration.report_detailed = false;
		}
		else if (option.name == "no-report-fixable") {
		    configuration.report_fixable = false;
		}
		else if (option.name == "no-report-missing") {
		    configuration.report_missing = false;
		}
		else if (option.name == "no-report-summary") {
		    configuration.report_summary = false;
		}
		else if (option.name == "report-correct") {
		    configuration.report_correct = true;
		}
		else if (option.name == "report-detailed") {
		    configuration.report_detailed = true;
		}
		else if (option.name == "report-fixable") {
		    configuration.report_fixable = true;
		}
		else if (option.name == "report-missing") {
		    configuration.report_missing = true;
		}
		else if (option.name == "report-summary") {
		    configuration.report_summary = true;
		}
		else if (option.name == "rom-dir") {
		    configuration.rom_directory = option.argument;
		}
		else if (option.name == "roms-unzipped") {
		    configuration.roms_zipped = false;
		}
		else if (option.name == "verbose") {
		    configuration.verbose = true;
		}
		else {
		    // TODO: report unhandled option
		}
	    }
	    
	    arguments = args.arguments;
	}
	catch (Exception &ex) {
	    commandline.usage(false, stderr);
	    exit(1);
	}

	if (!configuration.fix_romset) {
	    archive_global_flags(ARCHIVE_FL_RDONLY, true);
	}

	ensure_dir(configuration.rom_directory, false);
	std::error_code ec;
	rom_dir_normalized = std::filesystem::relative(configuration.rom_directory, "/", ec);
	if (ec || rom_dir_normalized.empty()) {
	    /* TODO: treat as warning only? (this exits if any ancestor directory is unreadable) */
	    myerror(ERRSTR, "can't normalize directory '%s'", configuration.rom_directory.c_str());
	    exit(1);
	}

	try {
	    CkmameDB::register_directory(configuration.rom_directory);
	    CkmameDB::register_directory(needed_dir);
	    CkmameDB::register_directory(unknown_dir);
	    for (const auto &name : configuration.extra_directories) {
		if (contains_romdir(name)) {
		    /* TODO: improve error message: also if extra is in ROM directory. */
		    myerror(ERRDEF, "current ROM directory '%s' is in extra directory '%s'", configuration.rom_directory.c_str(), name.c_str());
		    exit(1);
		}
		CkmameDB::register_directory(name);
	    }
	}
	catch (Exception &exception) {
	    exit(1);
	}

	try {
	    db = std::make_unique<RomDB>(configuration.romdb_name, DBH_READ);
	}
	catch (std::exception &e) {
	    myerror(0, "can't open database '%s': %s", configuration.romdb_name.c_str(), e.what());
	    exit(1);
	}
	try {
	    old_db = std::make_unique<RomDB>(configuration.olddb_name, DBH_READ);
	}
	catch (std::exception &e) {
	    /* TODO: check for errors other than ENOENT */
	}

	if (auto_fixdat || !configuration.fixdat.empty()) {
	    DatEntry de;

	    if (auto_fixdat) {
		if (!configuration.fixdat.empty()) {
		    myerror(ERRDEF, "do not use --autofixdat and --fixdat together");
		    exit(1);
		}

		auto d = db->read_dat();

		if (d.empty()) {
		    myerror(ERRDEF, "database error reading /dat");
		    exit(1);
		}

		configuration.fixdat = "fix_" + d[0].name + " (" + d[0].version + ").dat";
	    }

	    de.name = "Fixdat";
	    de.description = "Fixdat by ckmame";
	    de.version = "1";

	    if ((fixdat = OutputContext::create(OutputContext::FORMAT_DATAFILE_XML, configuration.fixdat, 0)) == nullptr) {
		exit(1);
	    }

	    fixdat->header(&de);
	}

	if (!configuration.roms_zipped && db->has_disks() == 1) {
	    fprintf(stderr, "%s: unzipped mode is not supported for ROM sets with disks\n", getprogname());
	    exit(1);
	}

	/* build tree of games to check */
	std::vector<std::string> list;
	
	try {
	    list = db->read_list(DBH_KEY_LIST_GAME);
	}
	catch (Exception &e) {
	    myerror(ERRDEF, "list of games not found in database '%s': %s", configuration.romdb_name.c_str(), e.what());
	    exit(1);
	}
	std::sort(list.begin(), list.end());

	if (!game_list.empty()) {
	    char b[8192];

	    seterrinfo(game_list);

	    auto f = make_shared_file(game_list, "r");
	    if (!f) {
		myerror(ERRZIPSTR, "cannot open game list");
		exit(1);
	    }

	    while (fgets(b, sizeof(b), f.get())) {
		if (b[strlen(b) - 1] == '\n')
		    b[strlen(b) - 1] = '\0';
		else {
		    myerror(ERRZIP, "overly long line ignored");
		    continue;
		}

		if (std::binary_search(list.begin(), list.end(), b)) {
		    check_tree.add(b);
		}
		else {
		    myerror(ERRDEF, "game '%s' unknown", b);
		}
	    }
	}
	else if (arguments.empty()) {
	    for (const auto &name : list) {
		check_tree.add(name);
	    }
	}
	else {
	    for (const auto &argument : arguments) {
		if (strcspn(argument.c_str(), "*?[]{}") == argument.size()) {
		    if (std::binary_search(list.begin(), list.end(), argument)) {
			check_tree.add(argument);
		    }
		    else {
			myerror(ERRDEF, "game '%s' unknown", argument.c_str());
		    }
		}
		else {
		    found = 0;
		    for (const auto & j : list) {
			if (fnmatch(argument.c_str(), j.c_str(), 0) == 0) {
			    check_tree.add(j);
			    found = 1;
			}
		    }
		    if (!found)
			myerror(ERRDEF, "no game matching '%s' found", argument.c_str());
		}
	    }
	}

	MemDB::ensure();
	
	if (!superfluous_delete_list) {
	    superfluous_delete_list = std::make_shared<DeleteList>();
	}
	superfluous_delete_list->add_directory(configuration.rom_directory, true);

	if (configuration.fix_romset) {
	    ensure_extra_maps(DO_MAP | DO_LIST);
	}

    #ifdef SIGINFO
	signal(SIGINFO, sighandle);
    #endif

	check_tree.traverse();
	check_tree.traverse(); /* handle rechecks */

	if (configuration.fix_romset) {
	    if (!needed_delete_list) {
		needed_delete_list = std::make_shared<DeleteList>();
	    }
	    if (needed_delete_list->archives.empty()) {
		needed_delete_list->add_directory(needed_dir, false);
	    }
	    cleanup_list(superfluous_delete_list, CLEANUP_NEEDED | CLEANUP_UNKNOWN);
	    cleanup_list(needed_delete_list, CLEANUP_UNKNOWN, true);
	}

	if (fixdat) {
	    fixdat->close();
	}

	if (configuration.fix_romset && configuration.move_from_extra) {
	    cleanup_list(extra_delete_list, 0);
	}

	if (arguments.empty()) {
	    print_superfluous(superfluous_delete_list);
	}
	
	if (configuration.report_summary) {
	    stats.print(stdout, false);
	}

	CkmameDB::close_all();
	
	if (configuration.fix_romset) {
	    std::error_code ec;
	    std::filesystem::remove(needed_dir, ec);
	}

	db = nullptr;
	old_db = nullptr;

	return 0;
    }
    catch (const std::exception &e) {
	fprintf(stderr, "%s: unexpected error: %s\n", getprogname(), e.what());
	exit(1);
    }
}


static bool
contains_romdir(const std::string &name) {
    std::error_code ec;
    std::string normalized = std::filesystem::relative(name, "/", ec);
    if (ec || normalized.empty()) {
	return false;
    }

    size_t length = std::min(normalized.length(), rom_dir_normalized.length());
    return (normalized.substr(0, length) == rom_dir_normalized.substr(0, length));
}
