#pragma once

#include "netascii_formatter.hpp"

#include <fstream>
#include <functional>
#include <vector>

class file_wrapper {
    std::fstream file_stream;
    uint32_t blksize;
    bool netascii = false;
    std::vector<unsigned char> buffer;
    netascii_formatter formatter;
public:
    file_wrapper() {
        this->blksize = 0;
        buffer.resize(0);
    }
    //All functions below return 1 on success and 0 on failure.
    
    //Sets everything up for append_content or write_content to be called with sequential packets
    bool open(std::string& filepath, uint16_t blksize, bool netascii, bool trunc);
    
    //function pointer that either redirects to append_content_netascii or append_content_octet
    std::function<bool(std::vector<uint8_t>& data)> append_content;
    //function pointer that either redirects to write_content_netascii or write_content_octet
    std::function<bool(std::vector<uint8_t>& data)> write_content;
    
    //variants of the above two functions for netascii and octet
    bool append_content_octet(std::vector<unsigned char>& data);
    bool write_content_octet(std::vector<unsigned char>& data);
    
    bool append_content_netascii(std::vector<unsigned char>& data);
    bool write_content_netascii(std::vector<unsigned char>& data);
};
