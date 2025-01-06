#pragma once

#include "BaseVcfManager.hh"

struct first_vcf_parse_result {
    parse_result result_to_write;
    std::shared_ptr<char> site_info_to_check;
    int site_info_len;
    std::shared_ptr<char> fmt_to_check;
    int fmt_len;
    int fmt_field_lookahead;

    first_vcf_parse_result(parse_result res, std::shared_ptr<char> site_info_to_check, int site_info_len,
                            std::shared_ptr<char> fmt_to_check, int fmt_len, int fmt_field_lookahead) : result_to_write(res), 
                            site_info_to_check(site_info_to_check), site_info_len(site_info_len),
                            fmt_to_check(fmt_to_check), fmt_len(fmt_len), fmt_field_lookahead(fmt_field_lookahead) {}
};

class FirstVcfManager: public BaseVcfManager {
    private:
        char * find_nth_tab_while_saving_line_start(char * &start, int n);
        
        bool found_chrom_line;

    public:
        FirstVcfManager(std::string vcf_name, std::shared_ptr<ThreadPool> thread_pool): BaseVcfManager(vcf_name, thread_pool), found_chrom_line(false){};

        parse_result parse_header();

        parse_result continue_parsing_header();

        first_vcf_parse_result parse_next_line();
};