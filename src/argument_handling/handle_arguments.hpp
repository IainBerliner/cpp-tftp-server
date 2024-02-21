#ifndef HANDLE_ARGUMENTS_HPP
#define HANDLE_ARGUMENTS_HPP

#include <string>

struct arguments_struct {
    std::string port;
    std::string tftp_root;
    std::string ops;
    bool allow_leaving_tftp_root;
    #ifdef TESTING_BUILD
    float bitflip;
    float drop;
    float duplicate;
    #endif
};
void quit_with_usage_message();
arguments_struct get_default_arguments();
void handle_arguments(int argc, char** argv, arguments_struct& input_arguments_struct);
#endif
