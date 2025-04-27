#pragma once

#include "../../macros/constants.hpp"

#include <cstdint>
#include <functional>
#include <vector>

#include <string.h>

class netascii_formatter {
    //the current buffer to be formatted
    std::vector<uint8_t> cur_buffer;
    //the next_buffer that will become the cur_buffer
    std::vector<uint8_t> next_buffer;
    //holds formatted bytes
    std::vector<uint8_t> output_buffer;
    
    /*
    //function pointer either to format_buffer_to_netascii or
    //format_buffer_to_hostascii, set with the set_mode call.
    */
    std::function<void(void)> format_buffer;
    
    //formats buffer to netascii
    void format_buffer_to_netascii();
    //formats buffer to hostascii
    void format_buffer_to_hostascii();
    
    /*
    //function pointer to either shuffle_buffer_reading or
    //shuffle_buffer_writing, set with the set_mode call.
    */
    std::function<void(const std::vector<uint8_t>&)> shuffle_buffer;
    
    //moves next_buffer into cur_buffer and bytes into next_buffer.
    void shuffle_buffer_reading(const std::vector<uint8_t>& bytes);
    //same as shuffle_buffer_reading but ignores first 4 bytes
    void shuffle_buffer_writing(const std::vector<uint8_t>& bytes);
public:
    /*
    //sets mode to writing or reading (writing means that the buffer must be converted to hostascii,
    //reading means that the buffer must be converted to netascii).
    */
    void set_mode(bool is_writing_mode);
    
    //formats cur_buffer and then calls shuffle_buffer with bytes
    void put(const std::vector<uint8_t>& bytes);
    //checks if there are enough bytes in output_buffer to read numbytes
    bool can_get(uint32_t numbytes);
    //get at most numbytes from output_buffer and also erase them
    std::vector<uint8_t> get(uint32_t numbytes);
};
