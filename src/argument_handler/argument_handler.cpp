#include "argument_handler.hpp"

#include <iostream>
#include <unordered_map>

void argument_handler::set_argument(std::string argument, int32_t value) {
    if (!int_arguments.contains(argument)) {
        all_arguments.push_back(argument);
    }
    else if (upper_bounds.contains(argument) && value > upper_bounds[argument]) {
        this->quit_with_usage_message();
    }
    else if (lower_bounds.contains(argument) && value < lower_bounds[argument]) {
        this->quit_with_usage_message();
    }
    int_arguments[argument] = value;
}

void argument_handler::set_argument(std::string argument, std::string value) {
    if (!string_arguments.contains(argument)) {
        all_arguments.push_back(argument);
    }
    string_arguments[argument] = value;
}

void argument_handler::set_argument(std::string argument, double value) {
    if (!double_arguments.contains(argument)) {
        all_arguments.push_back(argument);
    }
    else if (upper_bounds.contains(argument) && value > upper_bounds[argument]) {
        this->quit_with_usage_message();
    }
    else if (lower_bounds.contains(argument) && value < lower_bounds[argument]) {
        this->quit_with_usage_message();
    }
    double_arguments[argument] = value;
}

void argument_handler::set_upper_bound(std::string argument, double bound) {
    upper_bounds[argument] = bound;
}

void argument_handler::set_lower_bound(std::string argument, double bound) {
    lower_bounds[argument] = bound;
}

bool argument_handler::is_argument(std::string str) {
    if(str.size() > 2 && str[0] == '-' && str[1] == '-') {
        if (!int_arguments.contains(str) && !string_arguments.contains(str)
            && !double_arguments.contains(str)) { this->quit_with_usage_message(); }
        return true;
    }
    return false;
}

void argument_handler::process(std::string argument, std::string value) {
    if (int_arguments.contains(argument)) {
        int32_t converted_value;
        try { converted_value = std::stoi(value); }
        catch (...) { this->quit_with_usage_message(); }
        this->set_argument(argument, converted_value);
    }
    else if (string_arguments.contains(argument)) {
        this->set_argument(argument, value);
    }
    else if (double_arguments.contains(argument)) {
        double converted_value;
        try { converted_value = std::stod(value); }
        catch (...) { this->quit_with_usage_message(); }
        this->set_argument(argument, converted_value);
    }
    else {
        this->quit_with_usage_message();
    }
}

void argument_handler::process(int argc, char** argv) {
    std::string prev = "";
    for (int i = 1; i < argc; i++) {
        std::string str(argv[i]);
        if (str == "--help") { this->quit_with_usage_message(); }
        if (is_argument(str)) { prev = str; }
        else { process(prev, str); prev = ""; }
    }
    //quit with usage message if argument provided without value
    if (prev != "") { this->quit_with_usage_message(); }
}

uint64_t argument_handler::get_int(std::string arg) {
    return int_arguments["--" + arg];
}

std::string argument_handler::get_string(std::string arg) {
    return string_arguments["--" + arg];
}

double argument_handler::get_double(std::string arg) {
    return double_arguments["--" + arg];
}

void argument_handler::print_int_argument(std::string arg) {
    //special case: boolean argument
    if (lower_bounds[arg] == 0 && upper_bounds[arg] == 1) {
        std::cerr << "bool=0,1";
        return;
    }
    if (lower_bounds.contains(arg)) {
        std::cerr << int(lower_bounds[arg]) << "<="; 
    }
    std::cerr << "int";
    if (upper_bounds.contains(arg)) {
        std::cerr << "<=" << int(upper_bounds[arg]); 
    }
}

void argument_handler::print_string_argument(std::string arg) {
    for (uint32_t i = 2; i < arg.size(); i++) { std::cerr << arg[i]; }
}

void argument_handler::print_double_argument(std::string arg) {
    if (lower_bounds.contains(arg)) {
        std::cerr << lower_bounds[arg] << "<="; 
    }
    std::cerr << "double";
    if (upper_bounds.contains(arg)) {
        std::cerr << "<=" << upper_bounds[arg]; 
    }
}

void argument_handler::quit_with_usage_message() {
    std::cerr << "usage: ";
    for (std::string& argument : all_arguments) {
        std::cerr << argument << " [";
        if (int_arguments.contains(argument)) { this->print_int_argument(argument); }
        else if (string_arguments.contains(argument)) { this->print_string_argument(argument); }
        else { this->print_double_argument(argument); }
        std::cerr << "] ";
    }
    std::cerr << std::endl;
    exit(1);
}
