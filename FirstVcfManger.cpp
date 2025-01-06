#include "FirstVcfManager.hh"

parse_result FirstVcfManager::parse_header() {
    read_next_chunk();
    char * chr_line_start = strnstr(processing_buffer_start.get(), "#CHROM", processing_buffer_end - processing_buffer_start.get());
    if (!chr_line_start) {
        return parse_result(processing_buffer_start.get(), processing_buffer_end - processing_buffer_start.get(), false);
    }

    found_chrom_line = true;
    return parse_rest_of_line(chr_line_start, processing_buffer_start.get());
}

parse_result FirstVcfManager::continue_parsing_header() {
    if (!found_chrom_line) {
        return parse_header();
    }

    read_next_chunk();
    return parse_rest_of_line(processing_buffer_start.get());
}

char * FirstVcfManager::find_nth_tab_while_saving_line_start(char * &start, int n) {
    char * res = find_nth_tab(start, n);
    if (!res) {
        if (line_start == processing_buffer_start.get()) {
            throw std::runtime_error("Cannot continue searching for nth tab while saving line_start, entire buffer is filled.");
        }
        int new_start_offset = processing_buffer_end - line_start;
        int start_offset = start - line_start;
        read_next_chunk_with_remaining(line_start);
        
        start = line_start + start_offset;
        res = find_nth_tab(line_start + new_start_offset, n);
        if (!res) {
            throw std::runtime_error("Cannot continue searching for nth tab while saving line_start, entire buffer is filled.");
        }
    }

    return res;
}

first_vcf_parse_result FirstVcfManager::parse_next_line() {
    
    char * site_info_to_check_end = find_nth_tab_while_saving_line_start(line_start, 5);
    site_info_to_check_end++;
    int site_info_len = site_info_to_check_end - line_start;

    char * fmt_to_check = find_nth_tab_while_saving_line_start(site_info_to_check_end, 3);
    int fmt_field_lookahead = fmt_to_check - line_start;

    // search for second because initial pointer is at tab beginning fmt field
    char * fmt_to_check_end = find_nth_tab_while_saving_line_start(fmt_to_check, 2);
    fmt_to_check_end++;
    int fmt_len = fmt_to_check_end  - fmt_to_check;

    //line_start will be moved to next line by parse_rest_of_line in \n is found, so need to store here
    char * site_info_check = line_start;

    parse_result sample_res = parse_rest_of_line(fmt_to_check_end, line_start);

    return first_vcf_parse_result(sample_res, std::shared_ptr<char>(processing_buffer_start, site_info_check), site_info_len,
                                    std::shared_ptr<char>(processing_buffer_start, fmt_to_check), fmt_len, fmt_field_lookahead); 
}