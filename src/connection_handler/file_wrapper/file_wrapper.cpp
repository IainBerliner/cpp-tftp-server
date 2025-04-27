#include "file_wrapper.hpp"

#include <functional>
#include <iostream>
#include <vector>

bool file_wrapper::open(std::string& filepath,unsigned short blksize,bool netascii,bool is_writing_mode) {
    if(file_stream.is_open()) { file_stream.close(); }
    this->blksize = blksize;
    this->netascii = netascii;
    this->formatter.set_mode(is_writing_mode);
    auto flags = is_writing_mode ? std::fstream::in | std::fstream::out | std::fstream::trunc : std::fstream::in | std::fstream::out;
    file_stream.open(filepath, flags);
    buffer.resize(blksize);
    
    //set this->append_content to be a function pointer to the appropriate version of the function
    if (!netascii) { this->append_content = std::bind(&file_wrapper::append_content_octet, this, std::placeholders::_1); }
    else { this->append_content = std::bind(&file_wrapper::append_content_netascii, this, std::placeholders::_1); }
    //same with this->write_content
    if (!netascii) { this->write_content = std::bind(&file_wrapper::write_content_octet, this, std::placeholders::_1); }
    else { this->write_content = std::bind(&file_wrapper::write_content_netascii, this, std::placeholders::_1); }
    
    return file_stream.is_open();
}

bool file_wrapper::append_content_octet(std::vector<uint8_t>& data) {
    file_stream.read((char*)buffer.data(), this->blksize);
    buffer.resize(file_stream.gcount());
    data.insert(data.end(), buffer.begin(), buffer.end());
    return buffer.size() > 0 || file_stream.eof();
}

bool file_wrapper::write_content_octet(std::vector<uint8_t>& data) {
   file_stream.write((char*)data.data() + 4, data.size() - 4);
   return file_stream.good();
}

bool file_wrapper::append_content_netascii(std::vector<uint8_t>& data) {
    while (!formatter.can_get(this->blksize) && !file_stream.eof()) {
        file_stream.read((char*)buffer.data(), this->blksize);
        buffer.resize(file_stream.gcount());
        formatter.put(buffer);
    }
    if (!formatter.can_get(this->blksize) && file_stream.eof()) { formatter.put({}); }
    
    std::vector<uint8_t> formatted_bytes = formatter.get(this->blksize);
    data.insert(data.end(), formatted_bytes.begin(), formatted_bytes.end());
    
    return formatted_bytes.size() > 0 || file_stream.eof();
}

bool file_wrapper::write_content_netascii(std::vector<uint8_t>& data) {
    formatter.put(data);
    if (data.size() != this->blksize + 4) { formatter.put({}); }
    std::vector<uint8_t> formatted_bytes;
    while (true) {
        std::vector<uint8_t> formatted_bytes = formatter.get(this->blksize);
        if (formatted_bytes.empty()) { break; }
        file_stream.write((char*)formatted_bytes.data(), formatted_bytes.size());
    }
    return file_stream.good();
}
