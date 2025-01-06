#include "BaseVcfManager.hh"
#include <ios>

static const int WINDOW_SIZE = BGZF_BLOCK_SIZE;

BaseVcfManager::BaseVcfManager(std::string vcf_name, std::shared_ptr<ThreadPool> thread_pool):
    vcf_file_name(vcf_name), tab_counter(0), more_to_read(true), thread_pool(thread_pool), line_start(NULL),
    processing_buffer_start(new char[WINDOW_SIZE]), reading_buffer_start(new char[WINDOW_SIZE]) 
{
    f_vcf = bgzf_open(vcf_file_name.c_str(),"r");
    read_next_chunk();
}

BaseVcfManager::~BaseVcfManager() {
    if (read_result.valid()) {
        read_result.wait();
    }
    bgzf_close(f_vcf);
}

void BaseVcfManager::read_next_chunk() {
    if (read_result.valid()) {
        read_result.wait();
    }
    swap_buffers();
    read_result = thread_pool->submit_task([this]{this->read_with_offset(0);});
}

void BaseVcfManager::read_with_offset(const int offset) {

    if (more_to_read) {
        const ssize_t read_bytes = bgzf_read(f_vcf, reading_buffer_start.get() + offset, 
                                            WINDOW_SIZE - offset);
        if ( read_bytes < 0) {
            throw std::ios_base::failure("Failed to read in data from " + vcf_file_name);
        }

        if (read_bytes == 0) {
            more_to_read = false;
        }

        reading_buffer_end = reading_buffer_start.get() + offset + read_bytes;
    }
}

void BaseVcfManager::read_next_chunk_with_remaining(char * &start_of_remaining) {
    if (read_result.valid()) {
        read_result.wait();
    }
    int remaining_bytes_read_buffer = swap_buffers_with_remaining(start_of_remaining);
    read_result = thread_pool->submit_task([this, remaining_bytes_read_buffer]{this->read_with_offset(remaining_bytes_read_buffer);});
}

void BaseVcfManager::swap_buffers() {
    std::swap(processing_buffer_start, reading_buffer_start);
    std::swap(processing_buffer_end, reading_buffer_end);
}

int BaseVcfManager::swap_buffers_with_remaining(char * &start_of_remaining) {
    int remaining_bytes = processing_buffer_end - start_of_remaining;
    memmove(processing_buffer_start.get(), start_of_remaining, remaining_bytes);
    int bytes_to_move_from_reading = std::min((int)(reading_buffer_end - reading_buffer_start.get()),
                                             WINDOW_SIZE - remaining_bytes);
    memcpy(processing_buffer_start.get() + remaining_bytes, reading_buffer_start.get(), bytes_to_move_from_reading);
    processing_buffer_end = processing_buffer_start.get() + remaining_bytes + bytes_to_move_from_reading;
    int bytes_remaining_read_buffer = reading_buffer_end - reading_buffer_start.get() - bytes_to_move_from_reading;
    memmove(reading_buffer_start.get(), reading_buffer_start.get() + bytes_to_move_from_reading, bytes_remaining_read_buffer);
    start_of_remaining = processing_buffer_start.get();
    return bytes_remaining_read_buffer;
}

char *
BaseVcfManager::strnstr(const char *s, const char *find, size_t slen)
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

char * BaseVcfManager::find_nth_tab(char * buffer, int n) {
    if (buffer == processing_buffer_end) {
        return nullptr;
    }
    if(*buffer == '\t') {
        tab_counter++;
    }

    while (tab_counter < n) {
        if (++buffer == processing_buffer_end) {
            return nullptr;
        }
        if (*buffer == '\t') {
            tab_counter++;
        }
    }
    tab_counter = 0;
    return buffer;
}

char * BaseVcfManager::find_next_newline(char * buffer) {
    if (buffer == processing_buffer_end) {
        return nullptr;
    }
    while (*buffer != '\n') {
        if (++buffer == processing_buffer_end) {
            return nullptr;
        }
    }
    return buffer;
}

char * BaseVcfManager::find_nearby(const char needle, char * haystack, const int max_search) {
    if (*haystack == needle) {
        return haystack;
    }

    char * haystack_up = haystack;
    char * haystack_down = haystack;

    int n_search = 0;
    while (n_search++ < max_search) {
        if (*(++haystack_up) == needle) {
            return haystack_up;
        }

        if (*(--haystack_down) == needle) {
            return haystack_down;
        }
    }

    //couldn't find it before max_search
    return nullptr;
}

char * BaseVcfManager::find_maybe_fmt_quick(int lookahead_guess) {
    int max_search = 10;
    char * search_start = line_start + lookahead_guess;
    if (search_start > processing_buffer_end) {
        return nullptr;
    }

    return find_nearby('\t', search_start, max_search);
}

parse_result BaseVcfManager::parse_rest_of_line(char * parse_start, char * result_start) {
    if (!result_start) {
        result_start = parse_start;
    }
    char * chr_line_end = find_next_newline(parse_start);
    if (chr_line_end) {
        line_start = chr_line_end + 1;
        return parse_result(result_start, chr_line_end - result_start, true);
    } else {
        return parse_result(result_start, processing_buffer_end - result_start, false);
    }
}

parse_result BaseVcfManager::continue_parsing_line() {
    read_next_chunk();
    return parse_rest_of_line(processing_buffer_start.get());
}
