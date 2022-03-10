//
// Created by Dieter Baron on 2022/03/10.
//

#ifndef OUTPUT_H
#define OUTPUT_H

#include <string>

#include "DB.h"
#include "printf_like.h"

class Output {
  public:
    Output();

    void set_header(std::string header);
    void set_subheader(std::string subheader);
    void message(const std::string& string) { message("%s", string.c_str()); }
    void message(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void message_verbose(const std::string& string) { message_verbose("%s", string.c_str()); }
    void message_verbose(const char* fmt, ...) PRINTF_LIKE(2, 3);
    
    void set_error_archive(std::string archive_name, std::string file_name);
    void set_error_database(DB* db);
    void set_error_file(std::string file_name);

    void error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);

    void archive_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    
    void archive_file_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_file_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_file_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);

    void file_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void file_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void file_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    
  private:
    std::string header;
    std::string subheader;
    bool first_header;
    bool header_done;
    bool subheader_done;
    
    std::string archive_name;
    std::string file_name;
    DB* db;

    void print_header();

    void print_message_v(const char* fmt, va_list va);
    void print_error_v(const char* fmt, va_list va, const std::string& prefix = "", const std::string& postfix ="");
    std::string prefix_archive_file();
    static std::string postfix_system();
};


#endif // OUTPUT_H
