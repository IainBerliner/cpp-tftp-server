#ifndef CONNECTION_OPTIONS_HPP
#define CONNECTION_OPTIONS_HPP
#include <map>
#include <string>
struct connection_options {
    unsigned short data_size;
    unsigned char timeout;
    std::string filename;
    std::string transfer_mode;
    std::string operation;
    std::string tftp_root;
    std::string permitted_operations;
    bool allow_leaving_tftp_root;
};
struct connection_options get_default_options();
void sanitize_options(std::map<std::string, int>& requested_options, struct connection_options input_options);
struct connection_options modify_options(struct connection_options input_options, std::map<std::string, int> requested_options);
#endif
