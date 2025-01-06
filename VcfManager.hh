#pragma once
#include "htslib/bgzf.h"
#include <string>
#include <thread>
#include "ThreadPool.hh"
#include "BaseVcfManager.hh"

class VcfManager: public BaseVcfManager {
    public:
        VcfManager(std::string vcf_name, std::shared_ptr<ThreadPool> thread_pool): BaseVcfManager(vcf_name, thread_pool){};

        parse_result parse_header();

        parse_result parse_next_line(const char * const site_info_to_check, int site_info_len,
                                    const char * const fmt_to_check, int fmt_len, int fmt_field_lookahead);
};