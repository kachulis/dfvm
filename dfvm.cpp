#include <iostream>
#include "htslib/bgzf.h"
#include "htslib/bgzf.h"
#include "htslib/hts.h"
#include <cstring>
#include "boost/program_options.hpp"


/*
 * This function exists on MacOS, but not Linux, so we define here
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
char *
strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}


static const int WINDOW_SIZE = BGZF_BLOCK_SIZE;

namespace po = boost::program_options;

const char * new_line_byte = "\n";

char * next_sample_fields_ptr(char * buffer) {
    int tab_counter = 0;
    int n = 0;
    while (tab_counter <9) {
        if (buffer[n++] == '\t') {
            tab_counter++;
        }
    }
    return buffer + n;
}

bool are_any_0(const size_t cs[], int n) {
    for (int i=0; i<n; i++) {
        if (cs[i] == 0) {
            return true;
        }
    }

    return false;
}

int main(int argc, char* argv[]) {
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
    BGZF* f_in_0 = bgzf_open(inputs[0].c_str(), "r");
    BGZF* f_in_others[n_inputs - 1];
    for (int i = 1; i<n_inputs; i++) {
        f_in_others[i-1] = bgzf_open(inputs[i].c_str(),"r");
    }
    BGZF * f_out = bgzf_open(output.c_str(), "w");
    char * buffer_0 = (char*)malloc(WINDOW_SIZE);
    char * buffer_others[n_inputs - 1]; 
    for (int i=0; i<n_inputs-1;i++) {
        buffer_others[i] = (char*)malloc(WINDOW_SIZE);
    }

    //first write out entire header from f_1
    char * buffer_ptr_0;
    char * buffer_ptr_others[n_inputs - 1];
    size_t c_0;
    size_t c_others[n_inputs - 1];
    while ((c_0 = bgzf_read(f_in_0, buffer_0, WINDOW_SIZE))) {
        if (c_0 < 0) {
            std::cerr << "Error reading from " << argv[1] << std::endl;
            return 1;
        }

        char * chr_line_start = strnstr(buffer_0, "#CHROM", c_0);

        if (chr_line_start) { // found #CHROM line
            int n=chr_line_start - buffer_0;
            while (buffer_0[n++] != '\n')
                continue;

            if (bgzf_write(f_out, buffer_0, n - 1) < 0) {
                std::cerr << "Could not write " << n-1 << " bytes: Error " << f_out->errcode << std::endl;
                return 1;
            }
            buffer_ptr_0 = buffer_0 + n;
            break;
        }
        else {
            if (bgzf_write(f_out, buffer_0, c_0) < 0) {
                std::cerr << "Could not write " << c_0 << " bytes: Error " << f_out->errcode << std::endl;
                return 1;
            }
        }
    }

    for (int i = 0; i<n_inputs - 1; i++) {
        while ((c_others[i] = bgzf_read(f_in_others[i], buffer_others[i], WINDOW_SIZE))) {
            if (c_others[i] < 0) {
                std::cerr << "Error reading from " << argv[1] << std::endl;
                return 1;
            }
            char * chr_line_start = strnstr(buffer_others[i], "#CHROM", c_others[i]);
            if (!chr_line_start) {
                continue;
            }

            char * sample_field_ptr = next_sample_fields_ptr(chr_line_start);

            int n = 0;
            while (sample_field_ptr[n++] != '\n') {
                continue;
            }
            if (bgzf_write(f_out, sample_field_ptr - 1, n) < 0) {
                std::cerr << "Could not write " << n + 1 << " bytes: Error " << f_out->errcode << std::endl;
                return 1;
            }
            buffer_ptr_others[i] = sample_field_ptr + n;
            break;
        }
    }

    //write new line
    if (bgzf_write(f_out, new_line_byte, 1) < 0) {
        std::cerr << "Could not write end of line byte: Error " << f_out->errcode << std::endl;
        return 1;
    }

    while(c_0 != 0 && !are_any_0(c_others, n_inputs - 1)) {
        int tab_counter = 0;
        int contig_pos_a1_a2_n;
        int n_0 = 0;
        int n_others[n_inputs - 1];
        memset(n_others, 0, sizeof(n_others));
        char * format_start_0;
        int format_n;
        char * sample_field_ptr_others[n_inputs - 1];

        while(true) {
            if (buffer_ptr_0 + n_0 == buffer_0 + c_0) {
                memmove(buffer_0, buffer_ptr_0, n_0);
                c_0 = bgzf_read(f_in_0, buffer_0 + n_0, WINDOW_SIZE - n_0) + n_0;
                if (c_0 < n_0) {
                    std::cerr << "Error reading from " << inputs[0] << std::endl;
                    return 1;
                }
                if (c_0 == 0) {
                    break;
                }
                format_start_0 = format_start_0 - buffer_ptr_0 + buffer_0;
                buffer_ptr_0 = buffer_0;
            }
            if(buffer_ptr_0[n_0] == '\n') {
                break;
            }
            if (buffer_ptr_0[n_0] == '\t') {
                tab_counter++;
                if (tab_counter == 5) {
                    contig_pos_a1_a2_n = n_0;
                }
                else if(tab_counter == 8) {
                    format_start_0 = buffer_ptr_0 + n_0;
                } else if (tab_counter == 9) {
                    format_n = buffer_ptr_0 + n_0 - format_start_0;
                }
            }
            n_0++;            
        }
        for (int i=0;i<n_inputs-1; i++) {
            tab_counter = 0;
            while(true) {
                if (buffer_ptr_others[i] + n_others[i] == buffer_others[i] + c_others[i]) {
                    memmove(buffer_others[i], buffer_ptr_others[i], n_others[i]);
                    c_others[i] = bgzf_read(f_in_others[i], buffer_others[i] + n_others[i], WINDOW_SIZE - n_others[i]) + n_others[i];
                    if (c_others[i] < n_others[i]) {
                        std::cerr << "Error reading from " << inputs[i+1] << std::endl;
                        return 1;
                    }
                    if (c_others[i] == 0) {
                        break;
                    }
                    sample_field_ptr_others[i] = sample_field_ptr_others[i] - buffer_ptr_others[i] + buffer_others[i];
                    buffer_ptr_others[i] = buffer_others[i];
                }
                if(buffer_ptr_others[i][n_others[i]] == '\n') {
                    break;
                }
                if (buffer_ptr_others[i][n_others[i]] == '\t') {
                    tab_counter++;
                    if(tab_counter == 8) {
                        if (buffer_ptr_others[i] + n_others[i] + format_n + 1 > buffer_others[i] + c_others[i]) {
                            int remaining_bytes = buffer_others[i] + c_others[i] - buffer_ptr_others[i];
                            memmove(buffer_others[i], buffer_ptr_others[i], remaining_bytes);
                            c_others[i] = bgzf_read(f_in_others[i], buffer_others[i] + remaining_bytes, WINDOW_SIZE - remaining_bytes) + remaining_bytes;
                            if (c_others[i] < remaining_bytes) {
                                std::cerr << "Error reading from " << inputs[i+1] << std::endl;
                                return 1;
                            }
                            if (c_others[i] == 0) {
                                break;
                            }
                            sample_field_ptr_others[i] = sample_field_ptr_others[i] - buffer_ptr_others[i] + buffer_others[i];
                            buffer_ptr_others[i] = buffer_others[i];
                        }
                        if (strncmp(format_start_0, buffer_ptr_others[i] + n_others[i], format_n + 1)) {
                            std::cerr << "Format fields differ" << std::endl;
                            return 1;
                        }
                    }
                    else if(tab_counter == 9) {
                        sample_field_ptr_others[i] = buffer_ptr_others[i] + n_others[i];
                    }
                }
                n_others[i]++;             
            }
        }

        if (c_0 == 0 || are_any_0(c_others, n_inputs - 1)) {
            break;
        }

        for (int i =0; i<n_inputs -1 ; i++) {
            if (strncmp(buffer_ptr_0, buffer_ptr_others[i], contig_pos_a1_a2_n + 1)) {
                std::cerr << "Sites differ" << std::endl;
                return 1;
            }
        }
        int total_bytes_this_line = n_0 + 1;
        for (int i=0; i<n_inputs - 1; i++) {
            total_bytes_this_line += buffer_ptr_others[i] + n_others[i] - sample_field_ptr_others[i];
        }

        if (total_bytes_this_line < BGZF_BLOCK_SIZE && bgzf_flush_try(f_out, total_bytes_this_line))  {// don't want block break in the middle of a line, if possible
            return 1;
        }

        if (bgzf_write(f_out, buffer_ptr_0, n_0) < 0) {
            std::cerr << "Could not write " << n_0-1 << " bytes: Error " << f_out->errcode << std::endl;
            return 1;
        }
        buffer_ptr_0 += n_0 + 1;
        
        for (int i=0; i<n_inputs - 1; i++) {
            int n_bytes = buffer_ptr_others[i] + n_others[i] - sample_field_ptr_others[i];
            if (bgzf_write(f_out, sample_field_ptr_others[i], n_bytes) < 0) {
                std::cerr << "Could not write " << n_bytes << " bytes: Error " << f_out->errcode << std::endl;
                return 1;
            }
            buffer_ptr_others[i] += n_others[i] + 1;
        }
        //write new line
        if (bgzf_write(f_out, new_line_byte, 1) < 0) {
            std::cerr << "Could not write end of line byte: Error " << f_out->errcode << std::endl;
            return 1;
        }
    }

    
    bgzf_close(f_out);
    for (int i=0; i<n_inputs - 1; i++) {
        bgzf_close(f_in_others[i]);
    }
    return 0;
}