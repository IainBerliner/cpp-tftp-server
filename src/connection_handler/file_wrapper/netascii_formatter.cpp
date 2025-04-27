#include "netascii_formatter.hpp"

#include <functional>
#include <iostream>
#include <string>

void netascii_formatter::set_mode(bool is_writing_mode) {
    if (is_writing_mode) { this->format_buffer = std::bind(&netascii_formatter::format_buffer_to_hostascii, this); }
    else { this->format_buffer = std::bind(&netascii_formatter::format_buffer_to_netascii, this); }
    if (is_writing_mode) { this->shuffle_buffer = std::bind(&netascii_formatter::shuffle_buffer_writing, this, std::placeholders::_1); }
    else { this->shuffle_buffer = std::bind(&netascii_formatter::shuffle_buffer_reading, this, std::placeholders::_1); }
}

bool match(const std::vector<uint8_t>& buffer, const std::string& pattern, uint64_t index) {
    if (index + pattern.size() - 1 >= buffer.size()) { return false; }
    for (uint32_t i = 0; i < pattern.size(); i++) {
        if (buffer[index + i] != pattern[i]) { return false; }
    }
    return true;
}

void netascii_formatter::format_buffer_to_netascii() {
    uint32_t N = cur_buffer.size();
    /*
    //insert enough characters from next_buffer to recognize instances of a carriage return across
    //packets.
    */
    for (uint32_t i = 0; i < strlen(PLATFORM_CARRIAGE_RETURN); i++) {
        if (i >= next_buffer.size()) { break; }
        cur_buffer.push_back(next_buffer[i]);
    }
    /*
    //starting_offset exists so characters that form carriage returns across packets are ignored. It
    //is initially set to 0.
    */
    static uint32_t starting_offset = 0;
    /*
    //iterate through the cur_buffer looking for instances of '\r' or the carriage return. All other
    //bytes are added to output_buffer unaltered.
    */
    for (uint32_t i = starting_offset; i < N; i++) {
        if (cur_buffer[i] == '\r') { output_buffer.push_back('\r'); output_buffer.push_back('\0'); }
        else if (match(cur_buffer, PLATFORM_CARRIAGE_RETURN, i)) { 
            output_buffer.push_back('\r'); output_buffer.push_back('\n');
            i += strlen(PLATFORM_CARRIAGE_RETURN) - 1;
        }
        else { output_buffer.push_back(cur_buffer[i]); }
    }
    /*
    //Now, the starting_offset is set either to 0 or the number of bytes of next_buffer that were
    //used in forming a carriage return with cur_buffer that got formatted to a netascii carriage
    //return in output_buffer.
    */
    starting_offset = 0;
    for (int32_t i = strlen(PLATFORM_CARRIAGE_RETURN) - 1; i >= 1; i--) {
        if (match(cur_buffer, PLATFORM_CARRIAGE_RETURN, N - i)) {
            starting_offset = strlen(PLATFORM_CARRIAGE_RETURN) - i; break;
        }
    }
}

void insert_host_carriage_return(std::vector<uint8_t>& data) {
    for (const auto& byte : std::string(PLATFORM_CARRIAGE_RETURN)) {
        data.push_back(byte);
    }
}

void netascii_formatter::format_buffer_to_hostascii() {
    uint32_t N = cur_buffer.size();
    /*
    //Add a single byte from next_buffer (if not empty) so that netascii carriage returns can be
    //recognized across two different packets.
    */
    if (!next_buffer.empty()) { cur_buffer.push_back(next_buffer[0]); }
    /*
    //starting_offset prevents the '\n' from being added to output_buffer if it formed a netascii
    //carriage return with the previous packet. It is initialized to 0.
    */
    static uint32_t starting_offset = 0;
    /*
    //Iterate through cur_buffer looking for instances of '\r\0' or a netascii carriage return. All
    //other bytes are added to output_buffer unaltered.
    */
    for (uint32_t i = starting_offset; i < N; i++) {
        if (match(cur_buffer, "\r\n", i)) { insert_host_carriage_return(output_buffer); i += 1; }
        else if (match(cur_buffer, "\r\0", i)) { output_buffer.push_back('\r'); i += 1; }
        else { output_buffer.push_back(cur_buffer[i]); }
    }
    //Set starting_offset either to 1 or 0 depending on whether this packet formed a netascii
    //carriage return or "\r\0" with the previous packet.
    if (match(cur_buffer, "\r\n", N-1) || match(cur_buffer, "\r\0", N-1)){starting_offset = 1;}
    else { starting_offset = 0; }
}

void netascii_formatter::shuffle_buffer_reading(const std::vector<uint8_t>& bytes) {
    cur_buffer = std::move(next_buffer);
    next_buffer = bytes;
}

void netascii_formatter::shuffle_buffer_writing(const std::vector<uint8_t>& bytes) {
    cur_buffer = std::move(next_buffer);
    //ignore first 4 bytes which contain the opcode and block number
    next_buffer = {};
    for (uint32_t i = 4; i < bytes.size(); i++) { next_buffer.push_back(bytes[i]); }
}

void netascii_formatter::put(const std::vector<uint8_t>& bytes) {
    this->shuffle_buffer(bytes);
    if (!cur_buffer.empty()) { this->format_buffer(); }
}

//Returns whether there are at least numbytes in the output buffer
bool netascii_formatter::can_get(uint32_t numbytes) {
    return output_buffer.size() >= numbytes;
}

//Returns at most numbytes from the output buffer and then erases them
std::vector<uint8_t> netascii_formatter::get(uint32_t numbytes) {
    std::vector<uint8_t> output;
    uint32_t N = numbytes < output_buffer.size() ? numbytes : output_buffer.size();
    for (uint32_t i = 0; i < N; i++) {
        output.push_back(output_buffer[i]);
    }
    output_buffer.erase(output_buffer.begin(), output_buffer.begin() + N);
    return output;
}
