#include "connection_options.hpp"

#include "tftp_error.hpp"
#include "../macros/constants.hpp"

#include <string.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>

//fill out boilerplate options and bounds checking
void connection_options::set_default_options() {
    this->set("blksize", 512);
    this->set_upper_bound("blksize", MAX_UDP_PACKET - 4);
    this->set_lower_bound("blksize", 8);
    this->set("timeout", 1);
    this->set_upper_bound("timeout", 255);
    this->set_lower_bound("timeout", 1);
    this->set("max_timeouts", 2);
    this->set("tsize", 0);
    this->set("rel_filepath", "");
    this->set("transfer_mode", "");
    this->set("operation", "");
    this->set("permitted_operations", "");
    this->set("tftp_root", "");
    #ifdef DEBUG_BUILD
    this->set("drop_rate", 0.0);
    this->set("duplicate_rate", 0.0);
    this->set("bitflip_rate", 0.0);
    #endif
    return;
}

uint64_t get_file_size_octet(const std::string& filepath) {
    struct stat stat_buf;
    int rc = stat(filepath.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

bool match_platform_carriage_return(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < strlen(PLATFORM_CARRIAGE_RETURN)) { return false; }
    for (uint32_t i = 0; i < strlen(PLATFORM_CARRIAGE_RETURN); i++) {
        if (buffer[i] != PLATFORM_CARRIAGE_RETURN[i]) { return false; }
    }
    return true;
}

uint64_t get_file_size_netascii(const std::string& filepath) {
    std::ifstream ifs(filepath, std::ios::binary|std::ios::in);
    if (!ifs.is_open()) { return 0; }
    
    int32_t diff = 2 - strlen(PLATFORM_CARRIAGE_RETURN);
    std::vector<uint8_t> buffer;
    uint64_t cnt = 0;
    uint32_t debug_carriage_count = 0;
    
    uint8_t byte;
    while (ifs.read((char*)&byte, sizeof(uint8_t))) {
        buffer.push_back(byte); cnt++;
        if (buffer.size() > strlen(PLATFORM_CARRIAGE_RETURN)) { buffer.erase(buffer.begin()); }
        if (match_platform_carriage_return(buffer)) { cnt += diff; buffer.clear(); debug_carriage_count += 1; }
    }
    
    return cnt;
}

void connection_options::set(std::string key, double value) {
    double_opts[key] = value;
}

void connection_options::set(std::string key, int32_t value) {
    this->set(key, uint64_t(value));
}

void connection_options::set(std::string key, uint64_t value) {
    if (upper_bounds.contains(key) && value > upper_bounds[key]) {
        throw tftp_error(8);
    }
    else if (lower_bounds.contains(key) && value < lower_bounds[key]) {
        throw tftp_error(8);
    }
    if (key == "tsize" && int_opts.contains("tsize")) {
        uint64_t file_size = 0;
        std::string full_path = this->get_string("tftp_root") + "/" + this->get_string("rel_filepath");
        if (this->get_string("transfer_mode") == "octet") { file_size = get_file_size_octet(full_path); }
        else if (this->get_string("transfer_mode") == "netascii") { file_size = get_file_size_netascii(full_path); }
        #ifdef DEBUG_BUILD
        std::cout << "connection_options::get_string: file_size: " << file_size << std::endl;
        #endif
        int_opts[key] = file_size;
        return;
    }
    int_opts[key] = value;
}

void connection_options::set_upper_bound(std::string key, uint64_t value) {
    upper_bounds[key] = value;
}

void connection_options::set_lower_bound(std::string key, uint64_t value) {
    lower_bounds[key] = value;
}

void connection_options::set(std::string key, std::string value) {
    string_opts[key] = value;
}

double connection_options::get_double(std::string key) {
    return double_opts[key];
}

uint64_t connection_options::get_int(std::string key) {
    return int_opts[key];
}

std::string& connection_options::get_string(std::string key) {
    return string_opts[key];
}

void connection_options::update(std::unordered_map<std::string, int32_t> requested_options) {
    for (auto& [key,value] : requested_options) {
        if (!int_opts.contains(key)) {
            #ifdef DEBUG_BUILD
            std::cout << "!int_opts.contains(" << key << "): ";
            std::cout << !int_opts.contains(key) << std::endl;
            #endif
            throw tftp_error(8);
        }
        else {
            this->set(key, value);
        }
    }
}

void connection_options::update(std::unordered_map<std::string, std::string> requested_options) {
    for (auto& [key,value] : requested_options) {
        if (!string_opts.contains(key)) {
            #ifdef DEBUG_BUILD
            std::cout << "!string_opts.contains(" << key << "): ";
            std::cout << !string_opts.contains(key) << std::endl;
            #endif
            throw tftp_error(8);
        }
        else {
            this->set(key, value);
        }
    }
}
