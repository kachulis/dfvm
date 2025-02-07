#pragma once
#include "htslib/bgzf.h"
#include <string>
#include <thread>
#include "ThreadPool.hh"

struct parse_result {
    char * ptr_to_write_from;
    size_t len_to_write;
    bool parse_complete;

    parse_result(char * ptr_to_write_from, size_t len_to_write, bool parse_complete) :
        ptr_to_write_from(ptr_to_write_from), len_to_write(len_to_write), parse_complete(parse_complete) {}
};

class BaseVcfManager {
    protected:
        std::shared_ptr<ThreadPool> thread_pool;
        std::future<void> read_result;
        BGZF* f_vcf;
        std::shared_ptr<char> processing_buffer_start;
        char * processing_buffer_end;
        std::shared_ptr<char> reading_buffer_start;
        char * reading_buffer_end;
        char * line_start;
        std::string vcf_file_name;
        int tab_counter;
        bool more_to_read;

        void read_next_chunk();
        void read_with_offset(const int offset);
        void read_next_chunk_with_remaining(char * &start_of_remaining);
        void swap_buffers();
        int swap_buffers_with_remaining(char * &start_of_remaining);
        char * strnstr(const char *s, const char *find, size_t slen);
        char * find_nth_tab(char * s, int n);
        char * find_next_newline(char * s);
        char * find_nearby(const char needle, char * haystack, const int max_search);
        char * find_maybe_fmt_quick(int lookahead_guess);

        parse_result parse_rest_of_line(char * parse_start, char * result_start = NULL);
    
    public:
        BaseVcfManager(std::string vcf_name, std::shared_ptr<ThreadPool> thread_pool);

        ~BaseVcfManager();

        virtual parse_result parse_header() = 0;

        parse_result continue_parsing_line();

        bool finished() {
            if (line_start != processing_buffer_end) {
                return false;
            }

            if (read_result.valid()) {
                read_result.wait();
            }
            return !more_to_read;
        }
};