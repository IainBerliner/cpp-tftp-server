#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class argument_handler {
    argument_handler() {
        //set defaults
        this->set_argument("--port", 3490);
        this->set_upper_bound("--port", 65535);
        this->set_lower_bound("--port", 0);
        this->set_argument("--tftp-root",".");
        this->set_argument("--enable-reading",1);
        this->set_upper_bound("--enable-reading",1);
        this->set_lower_bound("--enable-reading",0);
        this->set_argument("--enable-writing",0);
        this->set_upper_bound("--enable-writing",1);
        this->set_lower_bound("--enable-writing",0);
        this->set_argument("--allow-leaving-tftp-root", 1);
        this->set_upper_bound("--allow-leaving-tftp-root", 1);
        this->set_lower_bound("--allow-leaving-tftp-root", 0);
        #ifdef DEBUG_BUILD
        this->set_argument("--drop", 0.0);
        this->set_upper_bound("--drop", 1.0);
        this->set_lower_bound("--drop", 0.0);
        this->set_argument("--bitflip", 0.0);
        this->set_upper_bound("--bitflip", 1.0);
        this->set_lower_bound("--bitflip", 0.0);
        this->set_argument("--duplicate", 0.0);
        this->set_upper_bound("--duplicate", 1.0);
        this->set_lower_bound("--duplicate", 0.0);
        #endif
    }
    std::vector<std::string> all_arguments;
    std::unordered_map<std::string, int32_t> int_arguments;
    std::unordered_map<std::string, std::string> string_arguments;
    std::unordered_map<std::string, double> double_arguments;
    std::unordered_map<std::string, double> upper_bounds;
    std::unordered_map<std::string, double> lower_bounds;
    void set_argument(std::string argument, int32_t value);
    void set_argument(std::string argument, std::string value);
    void set_argument(std::string argument, double value);
    void set_upper_bound(std::string argument, double upper_bound);
    void set_lower_bound(std::string argument, double lower_bound);
    bool is_argument(std::string str);
    void process(std::string argument, std::string value);
    void print_int_argument(std::string argument);
    void print_string_argument(std::string argument);
    void print_double_argument(std::string argument);
public:
    static argument_handler& instance() {
        static argument_handler INSTANCE;
        return INSTANCE;
    }
    void process(int argc, char** argv);
    uint64_t get_int(std::string argument);
    std::string get_string(std::string argument);
    double get_double(std::string argument);
    void quit_with_usage_message();
};
