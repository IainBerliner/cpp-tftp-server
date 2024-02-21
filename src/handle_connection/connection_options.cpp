#include "connection_options.hpp"

#include <sys/stat.h>

#include <iostream>
#include <map>
#include <string>

long get_file_size(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

//return a connection_options struct filled out with boilerplate options
struct connection_options get_default_options() {
    struct connection_options new_options;
    new_options.data_size = 512;
        new_options.timeout = 1;
        new_options.filename = std::string("");
        new_options.transfer_mode = std::string("none");
        new_options.operation = std::string("none");
        new_options.permitted_operations = std::string("none");
        new_options.tftp_root = std::string("none");
        return new_options;
}

//If the client requests an option value outside of the standards-specified values,
//this function clips it to a standards-specified value.
void sanitize_options(std::map<std::string, int>& requested_options, struct connection_options input_options) {
    auto i = requested_options.begin();
    while (i != requested_options.end()) {
        if (i->first == "blksize") {
            if (i->second < 8) {
                i->second = 8;
            }
            else if (i->second > 65535) {
                i->second = 65535;
            }
            i++;
        }
        else if (i->first == "timeout") {
            if (i->second < 1) {
                i->second = 1;
            }
            else if (i->second > 255) {
                i->second = 255;
            }
            i++;
        }
        else if (i->first == "tsize") {
            //Tell client filesize if request is RRQ
            if (input_options.operation == "RRQ") {
                i->second = get_file_size(input_options.filename);
                #ifdef TESTING_BUILD
                std::cout << "File size: " << i->second << std::endl;
                #endif
            }
            else if (input_options.operation == "WRQ") {
                //do nothing
            }
            i++;
        }
        else {
            i = requested_options.erase(i);
        }
    }
}

struct connection_options modify_options(struct connection_options input_options, std::map<std::string, int> requested_options) {
    for (auto i: requested_options) {
        if (i.first == "blksize") {
            input_options.data_size = i.second;
        }
        if (i.first == "timeout") {
            input_options.timeout = i.second;
        }
    }
    return input_options;
}


