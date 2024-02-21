#include "handle_arguments.hpp"

#include <iostream>
#include <map>

void quit_with_usage_message() {
    #ifdef TESTING_BUILD
    std::cerr << "usage: cxx-tftp-server-testing-build --port [int>=0] --tftp-root [directory] --ops [RRQ, WRQ, RRQ+WRQ] --allow-leaving-tftp-root" << std::endl;
    std::cerr << "--bitflip [1.0>=float>=0.0] --drop [1.0>=float>=0.0] --duplicate [1.0>=float>=0.0]" << std::endl;
    #endif
    #ifndef TESTING_BUILD
    std::cerr << "usage: cxx-tftp-server --port [int>=0] --tftp-root [directory] --ops [RRQ, WRQ, RRQ+WRQ] --allow-leaving-tftp-root" << std::endl;
    #endif
    exit(1);
}
arguments_struct get_default_arguments() {
    arguments_struct defaults;
    defaults.port = std::string("3490");
    defaults.tftp_root = std::string(".");
    defaults.ops = std::string("RRQ");
    defaults.allow_leaving_tftp_root = false;
    #ifdef TESTING_BUILD
    defaults.bitflip = 0.0;
    defaults.drop = 0.0;
    defaults.duplicate = 0.0;
    #endif
    return defaults;
}
#define VALUED_ARGUMENT_START_MACRO()                                                            \
if (i + 1 == argc) {                                                                             \
    std::cerr << "error: cli argument requiring value was passed without a value." << std::endl; \
    quit_with_usage_message();                                                                   \
}                                                                                                \
else if (already_handled.count(std::string(argv[i]))) {                                          \
    std::cerr << "error: cli argument was provided twice." << std::endl;                         \
    quit_with_usage_message();                                                                   \
}

#define VALUED_ARGUMENT_END_MACRO()           \
already_handled[std::string(argv[i])] = true; \
i = i + 2;

#define UNVALUED_ARGUMENT_START_MACRO()                                   \
if (already_handled.count(std::string(argv[i]))) {                        \
    std::cerr << "error: cli argument was provided twice." << std::endl;  \
    quit_with_usage_message();                                            \
}

#define UNVALUED_ARGUMENT_END_MACRO()         \
already_handled[std::string(argv[i])] = true; \
i = i + 1;

void handle_arguments(int argc, char** argv, arguments_struct& input_arguments_struct) {
    std::map<std::string, bool> already_handled;
    int i = 1;
    while (i < argc) {
        //std::cout << "arg: " << argv[i] << std::endl;
        if (std::string(argv[i]) == std::string("--help") || std::string(argv[i]) == std::string("help")) {
            quit_with_usage_message();
        }
        #ifdef TESTING_BUILD
        else if (std::string(argv[i]) == std::string("--drop")) {
            VALUED_ARGUMENT_START_MACRO()
            float value;
            try {
                value = std::stof(std::string(argv[i + 1]));
            }
            catch (...) {
                quit_with_usage_message();
            }
            if (!(value <= 1.0 && value >= 0.0)) {
                std::cerr << "error: value for argument " << "\"" << argv[i] << "\"" << " is out of range." << std::endl;
                quit_with_usage_message();
            }
            input_arguments_struct.drop = value;
            VALUED_ARGUMENT_END_MACRO();
        }
        else if (std::string(argv[i]) == std::string("--duplicate")) {
            VALUED_ARGUMENT_START_MACRO()
            float value;
            try {
                value = std::stof(std::string(argv[i + 1]));
            }
            catch (...) {
                quit_with_usage_message();
            }
            if (!(value <= 1.0 && value >= 0.0)) {
                std::cerr << "error: value for argument " << "\"" << argv[i] << "\"" << " is out of range." << std::endl;
                quit_with_usage_message();
            }
            input_arguments_struct.duplicate = value;
            VALUED_ARGUMENT_END_MACRO()
        }
        else if (std::string(argv[i]) == std::string("--bitflip")) {
            VALUED_ARGUMENT_START_MACRO()
            float value;
            try {
                value = std::stof(std::string(argv[i + 1]));
            }
            catch (...) {
                quit_with_usage_message();
            }
            if (!(value <= 1.0 && value >= 0.0)) {
                std::cerr << "error: value for argument " << "\"" << argv[i] << "\"" << " is out of range." << std::endl;
                quit_with_usage_message();
            }
            input_arguments_struct.bitflip = value;
            VALUED_ARGUMENT_END_MACRO()
        }
        #endif
        else if (std::string(argv[i]) == std::string("--port")) {
            VALUED_ARGUMENT_START_MACRO()
            std::string value;
            try {
                value = argv[i + 1];
                int value_as_int = std::stoi(value);
                if (value_as_int < 0) {
                    std::cerr << "error: value for argument " << "\"" << argv[i] << "\"" << " is out of range." << std::endl;
                    quit_with_usage_message();
                }
            }
            catch (...) {
                quit_with_usage_message();
            }
            input_arguments_struct.port = value;
            VALUED_ARGUMENT_END_MACRO()
        }
        else if (std::string(argv[i]) == std::string("--tftp-root")) {
            VALUED_ARGUMENT_START_MACRO()
            std::string value;
            try {
                value = argv[i + 1];
            }
            catch (...) {   
                quit_with_usage_message();
            }
            input_arguments_struct.tftp_root = value;
            VALUED_ARGUMENT_END_MACRO()
        }
        else if (std::string(argv[i]) == std::string("--ops")) {
            VALUED_ARGUMENT_START_MACRO()
            std::string value;
            try {
                value = argv[i + 1];
            }
            catch (...) {   
                quit_with_usage_message();
            }
            if (value != std::string("RRQ") && value != std::string("WRQ") && value != std::string("RRQ+WRQ") && value != std::string("WRQ+RRQ")) {
                quit_with_usage_message();
            }
            if (value == std::string("WRQ+RRQ")) {
                value = std::string("RRQ+WRQ");
            }
            input_arguments_struct.ops = value;
            VALUED_ARGUMENT_END_MACRO()
        }
        else if (std::string(argv[i]) == std::string("--allow-leaving-tftp-root")) {
            UNVALUED_ARGUMENT_START_MACRO()
            //this parameter doesn't need any arguments.
            input_arguments_struct.allow_leaving_tftp_root = true;
            UNVALUED_ARGUMENT_END_MACRO()
        }
        else {
            std::cerr << "error: unrecognized cli argument: \"" << argv[i] << "\"" << std::endl;
            quit_with_usage_message();
        }
    }
}
