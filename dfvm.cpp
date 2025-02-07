#include <iostream>
#include "htslib/bgzf.h"
#include "htslib/hts.h"
#include <cstring>
#include "boost/program_options.hpp"
#include <chrono>
#include <iomanip>
#include "VcfManager.hh"
#include "FirstVcfManager.hh"
#include "ThreadPool.hh"

void write(BGZF* fp, const void * buf, size_t len) {
    ssize_t written_bytes = bgzf_write(fp, buf, len);
    if (written_bytes < 0) {
        throw std::ios_base::failure("Could not write.");
    }
}

namespace po = boost::program_options;

const char * new_line_byte = "\n";

int main(int argc, char* argv[]) {
    //start clock for status
    const auto start_time = std::chrono::steady_clock::now();

    //setup command line params
    po::options_description desc("Allowed Options");
    desc.add_options()
        ("input,I", po::value< std::vector<std::string>>() ->composing()->required())
        ("output,O", po::value<std::string>() -> required())
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    std::vector< std::string> inputs = vm["input"].as<std::vector<std::string> >();
    std::string output = vm["output"].as<std::string>();
    const size_t n_inputs = inputs.size();
    
    //thread ppol for multithreaded read
    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>();

    //vcf managers for reading in and parsing data
    std::vector<std::shared_ptr<VcfManager>> vcf_managers;
    FirstVcfManager first_vcf_manager(inputs[0], pool);

    for (int i = 1; i<n_inputs; i++) {
        vcf_managers.push_back(std::make_shared<VcfManager>(inputs[i], pool));
    }

    //output file
    BGZF * f_out = bgzf_open(output.c_str(), "w");


    //parse headers
    parse_result header_res = first_vcf_manager.parse_header();
    write(f_out, header_res.ptr_to_write_from, header_res.len_to_write);
    while (!header_res.parse_complete) {
        header_res = first_vcf_manager.continue_parsing_header();
        write(f_out, header_res.ptr_to_write_from, header_res.len_to_write);

    }
   
    for (std::shared_ptr<VcfManager> vcf_manager : vcf_managers) {
        parse_result res = vcf_manager->parse_header();
        write(f_out, res.ptr_to_write_from, res.len_to_write);

        while (!res.parse_complete) {
            res = vcf_manager->continue_parsing_line();
            write(f_out, res.ptr_to_write_from, res.len_to_write);
        }
    }
    write(f_out, new_line_byte, 1);

    // counter for logging status
    int line_count=0;
    // modulo is very slow, so do this instead
    int next_print = line_count + 5'000'000;

    // parse lines of vcf
    while(!first_vcf_manager.finished()) {
        first_vcf_parse_result first_res = first_vcf_manager.parse_next_line();
        std::shared_ptr<char> site_info_to_check = first_res.site_info_to_check;
        std::shared_ptr<char> fmt_to_check = first_res.fmt_to_check;

        write(f_out, first_res.result_to_write.ptr_to_write_from, first_res.result_to_write.len_to_write);

        if (!first_res.result_to_write.parse_complete) {
            //copy over info to check since we will need to keep reading in the first file
            site_info_to_check.reset(new char[first_res.site_info_len]);
            memcpy(site_info_to_check.get(), first_res.site_info_to_check.get(), first_res.site_info_len);

            fmt_to_check.reset(new char[first_res.fmt_len]);
            memcpy(fmt_to_check.get(), first_res.fmt_to_check.get(), first_res.fmt_len);
            parse_result first_res_continued = first_vcf_manager.continue_parsing_line();
            write(f_out, first_res_continued.ptr_to_write_from, first_res_continued.len_to_write);
            while(!first_res_continued.parse_complete) {
                first_res_continued = first_vcf_manager.continue_parsing_line();
                write(f_out, first_res_continued.ptr_to_write_from, first_res_continued.len_to_write);
            }
        }

        for (std::shared_ptr<VcfManager> vcf_manager : vcf_managers) {
            parse_result res = vcf_manager->parse_next_line(site_info_to_check.get(), first_res.site_info_len, 
                                                            fmt_to_check.get(), first_res.fmt_len, first_res.fmt_field_lookahead);

            write(f_out, res.ptr_to_write_from, res.len_to_write);

            while (!res.parse_complete) {
                res = vcf_manager->continue_parsing_line();
                write(f_out, res.ptr_to_write_from, res.len_to_write);
            }
        }

        //log
        if (++line_count == next_print) {
            auto time_now = std::chrono::steady_clock::now();
            auto dur = time_now - start_time;
            auto h = std::chrono::duration_cast<std::chrono::hours>(dur);
            auto m = std::chrono::duration_cast<std::chrono::minutes>(dur -= h);
            auto s = std::chrono::duration_cast<std::chrono::seconds>(dur -= m);
            std::cout<<std::setfill('0')<<std::setw(2)<<h.count()<<":"<<std::setw(2)<<m.count()<<":"<<std::setw(2)<<s.count()<<", last position: ";
            std::cout.write(site_info_to_check.get(), first_res.site_info_len)<<std::endl;
            next_print += 5'000'000;
        }

        write(f_out, new_line_byte, 1);
    }
    
    bgzf_close(f_out);
    return 0;
}