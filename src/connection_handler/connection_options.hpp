#pragma once

#include <map>
#include <string>
#include <unordered_map>

class connection_options {
    std::unordered_map<std::string, double> double_opts;
    std::unordered_map<std::string, uint64_t> int_opts;
    std::unordered_map<std::string, uint64_t> upper_bounds;
    std::unordered_map<std::string, uint64_t> lower_bounds;
    std::unordered_map<std::string, std::string> string_opts;
    void set_default_options();
public:
    void set(std::string key, double value);
    void set(std::string key, int32_t value);
    void set(std::string key, uint64_t value);
    void set_upper_bound(std::string key, uint64_t upper_bound);
    void set_lower_bound(std::string key, uint64_t lower_bound);
    void set(std::string key, std::string value);
    double get_double(std::string key);
    uint64_t get_int(std::string key);
    std::string& get_string(std::string key);
    void update(std::unordered_map<std::string, int32_t> requested_options);
    void update(std::unordered_map<std::string, std::string> requested_options);
    connection_options() { this->set_default_options(); }
    ~connection_options() { }
};
