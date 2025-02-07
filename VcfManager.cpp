#include "VcfManager.hh"

parse_result VcfManager::parse_header() {
    read_next_chunk();
    char * chr_line_start = strnstr(processing_buffer_start.get(), "#CHROM", processing_buffer_end - processing_buffer_start.get());
    while(!chr_line_start) {
        read_next_chunk();
        chr_line_start = strnstr(processing_buffer_start.get(), "#CHROM", processing_buffer_end - processing_buffer_start.get());
    }

    char * sample_field_start = find_nth_tab(chr_line_start, 9);
    while (!sample_field_start) {
        read_next_chunk();
        sample_field_start = find_nth_tab(processing_buffer_start.get(), 9);
    }

    return parse_rest_of_line(sample_field_start);
}

parse_result VcfManager::parse_next_line(const char * const site_info_to_check, int site_info_len,
                                        const char * const fmt_to_check, int fmt_len, int fmt_field_lookahead) {
    
    if (processing_buffer_end - line_start < site_info_len) {
        read_next_chunk_with_remaining(line_start);
        line_start = processing_buffer_start.get();
    }

    //TODO: handle edge case where site_info_len is larger than entire data buffer.  
    //Super unlikely, but not handled correctly currently.

    //check that site is same
    if (memcmp(line_start, site_info_to_check, site_info_len)) {
        throw std::runtime_error("Site differ");
    }

    //find start of format field
    //start by trying a quick search
    char * fmt_field_start = find_maybe_fmt_quick(fmt_field_lookahead);
    if (!fmt_field_start || processing_buffer_end - fmt_field_start < fmt_len || memcmp(fmt_field_start, fmt_to_check, fmt_len)) {
        fmt_field_start = find_nth_tab(line_start + site_info_len, 3);
        while (!fmt_field_start) {
            read_next_chunk();
            fmt_field_start = find_nth_tab(processing_buffer_start.get(), 3);
        }

        if (processing_buffer_end - fmt_field_start < fmt_len) {
            read_next_chunk_with_remaining(fmt_field_start);
            fmt_field_start = processing_buffer_start.get();
        }

        //TODO: handle edge case where fmt_len is larger than entire data buffer.  
        //Super unlikely, but not handled correctly currently.

        if (memcmp(fmt_field_start, fmt_to_check, fmt_len)) {
            throw std::runtime_error("Fmt fields differ");
        }
    }

    char * sample_field_start = fmt_field_start + fmt_len - 1;
    return parse_rest_of_line(sample_field_start);
}
